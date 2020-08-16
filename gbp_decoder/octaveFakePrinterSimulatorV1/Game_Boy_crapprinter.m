% Raphaël BOICHOT 16-08-2020
% code to simulate the speckle aspect of real Game boy printer
clc;
clear;
% choose a 9*9 image of a thermal printer dot
DOT=imread('dot_small.png');
%choose your image
IMAGE=imread('Game_Boy_Printer_ frame_1.png');
%choose the error rate on each printer dot (Gaussian noise)
error_rate_printer_head=0.1;
%choose the error rate on each pixel (Gaussien noise)
error_rate_brownian=0.25;

figure(1)
imagesc(DOT(:,:,1))
intensity=double(DOT(:,:,1));
intensity_map=1-double(IMAGE(:,:,1))/255;
min_int=min(min(intensity));
max_int=max(max(intensity));
intensity=1-(intensity-min_int)/(max_int-min_int);
%intensity map for printer head with threshold
intensity=0.4+0.6*intensity;
figure(2)
surf(intensity)
[mul_h, mul_w,mul_d]=size(intensity);
[heigth, width,deepness]=size(IMAGE);
speckle_image=zeros(heigth*mul_h,width*mul_w);
for i=1:1:heigth
   for j=1:1:width
       a=(i-1)*mul_h+1;
       b=i*mul_h;
       c=(j-1)*mul_w+1;
       d=j*mul_w;
       %apply the printer head error
       burn_dot=error_rate_printer_head*intensity*intensity_map(i,j)*abs(randn)...
           +(1-error_rate_printer_head)*intensity*intensity_map(i,j);
       error_mask=abs(randn(mul_h,mul_w));
       %apply individual pixel error;
       burn_dot=error_rate_brownian*burn_dot.*error_mask+(1-error_rate_brownian)*burn_dot;  
       speckle_image(a:b,c:d)=burn_dot;    
   end
end



%A bit of Gaussian Blur
for i=2:1:heigth*mul_h-1
   for j=2:1:width*mul_w-1
    blur(i-1,j-1)=(speckle_image(i,j+1)+speckle_image(i,j-1)+...
        speckle_image(i+1,j)+speckle_image(i-1,j)+2*speckle_image(i,j))/6;
   end
end



speckle_image=uint8((1-blur)*255);
figure(3)
imagesc(speckle_image);
colormap gray
imwrite(speckle_image,'test.png')