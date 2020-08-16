% Raphaël BOICHOT 12-08-2020
% code can handle compression and multiple images

clc;
clear;
color_option=1; %2 for Black and white, 2 for Game Boy Color, 3 for Game Boy DMG
fid = fopen('New_format.txt','r');
IMAGE=[];
INIT=0;
DATA=0;
PRINTER=0;
PRINTING=0;

while ~feof(fid)
a=fgets(fid);
str='INIT';
if not(isempty(strfind(a,str)))
disp('INIT COMMAND')
INIT=1;
end

str='DATA';
if not(isempty(strfind(a,str)))
disp('DATA PACKET RECEIVED')
DATA=1;
end

% Deactivate these comments if the image does not print
% str='Timed Out';
% if not(isempty(strfind(a,str)))
% disp('PRINT DATA TO A NEW FILE DUE TO TIMEOUT')
% PRINTER=1;
% PRINTING=PRINTING+1;
% end


str='PRNT';
if not(isempty(strfind(a,str)))
a=fgets(fid);
% We get the current palette here
PALETTE=dec2bin(hex2dec(a(25:26)),8);
%3 = black, 2 = dark gray, 1 = light gray, 0 = white
COLORS=[bin2dec(PALETTE(7:8)), bin2dec(PALETTE(5:6))...
    bin2dec(PALETTE(3:4)), bin2dec(PALETTE(1:2))]
disp('PRINT DATA TO A NEW FILE DUE TO MARGIN')
MARGIN=dec2bin(hex2dec(a(22:23)),8);
if MARGIN(7:8)=='11';
PRINTER=1;
PRINTING=PRINTING+1;
end
end



if DATA==1
    %extract data packet
    a=fgets(fid);
    DATA_length=hex2dec([a(16:17),a(13:14)]);
        
    COMPRESSION=0;
    if a(10:11)=='01'; 
        COMPRESSION=1;
        disp('COMPRESSED DATA');
    end
           
    if not(COMPRESSION==1);
    DATA_tiles=DATA_length/16;
    disp(['This packet contains ',num2str(DATA_length),' bytes and ',num2str(DATA_tiles),' tiles']);
    end
    
if COMPRESSION==1;
    %in case on compression, I assume the decompressed data are 640 bytes
    %long
    %if data are compressed, uncompressed data are generated before
    %decoding. 
    b=[];
    pos=19;
    byte_counter=0;
    
    while byte_counter<640
    command=hex2dec(a(pos:pos+1));
    if command>128;%its a compressed run, read the next byte and repeat
        %disp('Compressed Run')
        length_run=command-128+2;
        byte=a(pos+3:pos+4);
        for i=1:1:length_run
            b=[b,byte,' '];
        end
        byte_counter=byte_counter+length_run;
        pos=pos+6;

    end
    
    if command<128;%its a classical run, read the n bytes after
        %disp('Uncompressed Run')
        length_run=command+1;
        byte=a(pos+3:pos+length_run*3+1);
        b=[b,byte,' '];
        byte_counter=byte_counter+length_run;
        pos=pos+length_run*3+3;
        
    end
    end
    a=b;
    DATA_tiles=40;
end
    
    %make a compact hexadecimal phrase without the protocol, just the data
        
    if not (COMPRESSION==1);
    a=a(19:end-15);
    end
    
    DATA_compact=a;
    PACKET_image_width=160;
    PACKET_image_height=8*DATA_tiles/20;
    PACKET_image=zeros(PACKET_image_height,PACKET_image_width);    
    pos=1;
    %tile decoder
    tile_count=0;
    height=1;
    width=1;
while tile_count<DATA_tiles    
    for i=1:1:8
    byte1=dec2bin(hex2dec([a(pos),a(pos+1)]),8);
    pos=pos+3;
    byte2=dec2bin(hex2dec([a(pos),a(pos+1)]),8);
    pos=pos+3;
    
      for j=1:1:8
      tile(i,j)=bin2dec([byte2(j),byte1(j)]);
      end
    end
    PACKET_image((height:height+7),(width:width+7))=tile;
    tile_count=tile_count+1;
    width=width+8;
      if width>=160
      width=1;
      height=height+8;     
      end
end
    DATA=0;
    IMAGE=[IMAGE;PACKET_image];
end    


if PRINTER==1
    figure(1)
    imagesc(IMAGE)
    colormap(gray)
    pause(1)
[h, w, d]=size(IMAGE);
frame=zeros(h, w, 3);
%now we swap the palette
        for j=1:1:h;
        for k=1:1:w;
        IMAGE(j,k)=COLORS(IMAGE(j,k)+1);
        end
        end

%now we colorize the image in RGB true colors        
        for j=1:1:h;
        for k=1:1:w;
if color_option==1
        if IMAGE(j,k)==0; frame(j,k,:)=[255 255 255]; end;
        if IMAGE(j,k)==1; frame(j,k,:)=[168 168 168]; end;
        if IMAGE(j,k)==2; frame(j,k,:)=[84 84 84]; end;
        if IMAGE(j,k)==3; frame(j,k,:)=[0 0 0]; end;
end

if color_option==2
         if IMAGE(j,k)==3; frame(j,k,:)=[0 19 26];end;
         if IMAGE(j,k)==2; frame(j,k,:)=[6 75 145]; end;
         if IMAGE(j,k)==1; frame(j,k,:)=[130 222 73]; end;
         if IMAGE(j,k)==0; frame(j,k,:)=[215 247 215]; end;
end

if color_option==3
         if IMAGE(j,k)==3; frame(j,k,:)=[56 110 86];end;
         if IMAGE(j,k)==2; frame(j,k,:)=[70 131 89]; end;
         if IMAGE(j,k)==1; frame(j,k,:)=[93 150 78]; end;
         if IMAGE(j,k)==0; frame(j,k,:)=[120 169 59]; end;
end
        end
        end
      
    frame=uint8(frame);
    figure(2)
    imagesc(frame)
    colormap(gray)
    pause(1)
    imwrite(frame,['Game_Boy_Printer_ frame_',num2str(PRINTING),'.png'])
PRINTER=0;
IMAGE=[];
end

end
