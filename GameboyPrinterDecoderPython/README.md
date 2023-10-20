
Python application to decode Gameboy Printer data and export png images.

Input for this application is Gameboy Printer Emulator raw packet dump that can be saved from its serial output.

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