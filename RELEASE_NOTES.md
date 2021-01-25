# Release Notes

All downloadable releases are now located in [github release folder for arduino gameboy printer emulator archive](https://github.com/mofosyne/arduino-gameboy-printer-emulator/releases)


-------------------------------------------------------------------------------

## V3 - 2021-01-26

Code is mostly the same, but GBP_OUTPUT_RAW_PACKETS is now true. 

This is because the javascript only decoder is able to decode compressed packets as well.

Tried to do decompression in the arduino nano, but was not able to. Such feature requires at least SAMD21, SAMD51, ESP8266, ESP32. If you want to use it, you should set GBP_OUTPUT_RAW_PACKETS to false and GBP_USE_PARSE_DECOMPRESSOR to true.

Also page has been simplified to only show V3, since V2 and V1 is now shown as seperate releases in https://github.com/mofosyne/arduino-gameboy-printer-emulator/releases

Maybe later I would move the C decompressor lib from the arduino code to a standalone PC implementation. But for now, this shall do.

-------------------------------------------------------------------------------

## V2 - 2020-08-16

### Gameboy Printer Emulator V2 

Second major release is a total rewrite focusing on extensibility and portability
of the code. Aside from the `.ino` file, all other files is written in pure C
code with no Arduino dependencies. You can run a test program in your computer
in the `./test/` folder to test both the serial io and packet parser.

Historically the test folder was used during coding to ensure that what was
written matches the serial capture from `gbp_sniffer` in the research folder.

Press `?` to open up menu of options.

New feature of this arduino code:

* Run Length Compression decoder (e.g. Pokemon Trading Card Game)
* Doublespeed support (e.g. Pokemon Trading Card Game)
* Correct status byte response (e.g. Pokemon Yellow)
* Raw packet output mode (Good for technical research purpose)

This means larger support for a wide variety of games with more advance printer
drivers e.g. pokemon trading card game.

### Gameboy Printer Emulator Tile Decoder V2 

This contains incremental improvements from the community to support multiple
photos in a single stream as well as to color the photos.

* [Open V2 JS Decoder](https://mofosyne.github.io/arduino-gameboy-printer-emulator/gbp_decoder/jsdecoderV2/gameboy_printer_js_decoder.html)


-------------------------------------------------------------------------------

## V1 - 2017-04-6

### Gameboy Printer Emulator V1

First release. Kept here due to simpler coding structure which would be good for
understanding the basics. Also to ensure support for anyone else who is still
using this output for their own apps.

### Gameboy Printer Emulator Tile Decoder V1

First release. Kept here due to simpler coding structure which would be good for
understanding the basics. And also with less checks, it's good for developments.

* [Open V1 JS Decoder](https://mofosyne.github.io/arduino-gameboy-printer-emulator/gbp_decoder/jsdecoderV1/gameboy_printer_js_decoder.html)


-------------------------------------------------------------------------------

## Other releases to note

### Octave/Matlab Fake Printer Simulator

Rapha�l BOICHOT contributed a decoder specifically written to simulate the
imperfection of a real printer. The development is quite fascinating and may be
of interest for gameboy emulator developers.

Source code located in https://github.com/mofosyne/GameboyPrinterPaperSimulation/

* [Click here to read the write up about this script and it's development](https://mofosyne.github.io/GameboyPrinterPaperSimulation/)
