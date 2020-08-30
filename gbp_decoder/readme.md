# Gameboy Printer Emulator Tile Decoder

This contains various decoder implementations you can use. For most people
the javascript decoder should be of most use.

## Javascript Tile Decoder

Written in javascript, this allows most users to copy over the arduino default
output into the web browser and render the image. This is the easiest

### V2 - 2020-08-16

This contains incremental improvements from the community to support multiple
photos in a single stream as well as to color the photos.

* [Open V2 JS Decoder](https://mofosyne.github.io/arduino-gameboy-printer-emulator/gbp_decoder/jsdecoderV2/gameboy_printer_js_decoder.html)


### V1 - 2017-04-6

First release. Kept here due to simpler coding structure which would be good for
understanding the basics. And also with less checks, it's good for developments.

* [Open V1 JS Decoder](https://mofosyne.github.io/arduino-gameboy-printer-emulator/gbp_decoder/jsdecoderV1/gameboy_printer_js_decoder.html)


## Octave/Matlab Fake Printer Simulator

Rapha�l BOICHOT contributed a decoder specifically written to simulate the
imperfection of a real printer. The development is quite fascinating and may be
of interest for gameboy emulator developers.

Source code located in https://github.com/mofosyne/GameboyPrinterPaperSimulation/

* [Click here to read the write up about this script and it's development](https://mofosyne.github.io/GameboyPrinterPaperSimulation/)
