
# Gameboy Printer decoder

Python application to decode Gameboy Printer data and export png images.

Input for this application is Gameboy Printer Emulator raw packet dump that can be saved from the serial output.

Required libraries

* PIL Python Imaging Library (https://pypi.org/project/pip/)
* numpy (https://pypi.org/project/numpy/)


### Usage

Application reads the input file and decodes it to a png file. Native output images are small so larger 2 x scaled image is also written for easier preview.

```
C:\projects\gameboy_printer_emulator\GameboyPrinterDecoderPython>python gbpdecoder.py -h
usage: gbpdecoder.py [-h] [--verbose] [-i FILE]

GameBoy Printer Data Decoder

options:
  -h, --help  show this help message and exit
  --verbose   Verbose mode
  -i FILE     Input hexfile in ASCII format
```

### Examples

Input file is processed and images are stored on the same folder.
```
C:\projects\gameboy_printer_emulator\GameboyPrinterDecoderPython>python gbpdecoder.py -i testdata\2020-08-10_Pokemon_trading_card_compressiontest.txt 
Wrote testdata\2020-08-10_Pokemon_trading_card_compressiontest.png
Wrote testdata\2020-08-10_Pokemon_trading_card_compressiontest-2x.png
```

Without arguments application reads data from the standard input.
Be aware that Microsoft Windows command prompt window does not support copy pasting long lines. The data packets may be truncated that will corrupt the images.

```
C:\projects\gameboy_printer_emulator\GameboyPrinterDecoderPython>python gbpdecoder.py < testdata\test1.txt
Wrote test1.png
Wrote test1-2x.png
```

# Gameboy Emulator Reader

Reader connects to Arduino serial port directly and listens print data online. Received images are written to output folder.
Data can also be optionally logged in text files.

Required libraries

* Python Serial Library (https://pypi.org/project/pyserial/)
* PIL Python Imaging Library (https://pypi.org/project/pip/)
* numpy (https://pypi.org/project/numpy/)


### Usage

```
usage: gbpemulator_reader.py [-h] [--verbose] [-d DIR] [-l] [-p PORT]

GameBoy Printer Emulator Reader reads image data over serial port and stores decoded images. Data can be additionally logged to text files.

options:
  -h, --help            show this help message and exit
  --verbose             verbose mode
  -d DIR, --dest DIR    Image output directory
  -l, --log             Log received data
  -p PORT, --port PORT  Serial port

```

### Example session. 
Start the application and issue print command from the Gameboy. App prints dot '.' for each packet received and after a timeout it attempts to decode the packets to images. Hash '#' is printed for any other  messages from the emulator.

```
C:\projects\gameboy_printer_emulator\GameboyPrinterDecoderPython>python gbpemulator_reader.py
Device port:  COM8
Output directory: output
Waiting for data...
#########.......................................##
Processing 39 packets
360 Tiles. Palette [3, 2, 1, 0]
160 x 144 (w x h) 23040 pixels
Wrote output\out1.png
.......................................##
Processing 39 packets
360 Tiles. Palette [3, 2, 1, 0]
160 x 144 (w x h) 23040 pixels
Wrote output\out2.png
```

### Troubleshooting

#### Emulator reader may fail to open the port.

Clone Arduino Nano boards using CH340 USB-to-Serial chip seem to have issues on Windows 11. I believe it's because of the later driver version that Windows 11 autoinstalls on plugin. 
Install manually older CH340 3.3.2011 drivers (https://jisotalo.github.io/others/CH340-drivers-3.3.2011.zip). 

You may need to change the device driver to this older version manually in Windows Device Manager. Windows 11 installs latest driver every time you plug the Nano to a new USB port.

```
serial.serialutil.SerialException: Cannot configure port, something went wrong. Original message: PermissionError(13, 'A device attached to the system is not functioning.', None, 31)
```

PermissionError happens when another program has the port. This is usually the Arduino IDE.

```
serial.serialutil.SerialException: ClearCommError failed (PermissionError(13, 'Access is denied.', None, 5))
```

#### Frequent checksum fails on data transfer. 
You may have poor wiring from Arduino to the cable. Also new CH340 driver can cause byte drops. 
```
Waiting for data...
#########..WARNING: Command 4. Checksum 0x137 does not match data.
..WARNING: Command 4. Checksum 0xc986 does not match data.
.WARNING: Command 4. Checksum 0xee91 does not match data.
..WARNING: Command 4. Checksum 0x8c2b does not match data.
.WARNING: Command 4. Checksum 0x5004 does not match data.
..WARNING: Command 4. Checksum 0x49ac does not match data.
.WARNING: Command 4. Checksum 0xb7e7 does not match data.
..WARNING: Command 4. Checksum 0x428b does not match data.
.WARNING: Command 4. Checksum 0x15d4 does not match data.
......#..................##
```