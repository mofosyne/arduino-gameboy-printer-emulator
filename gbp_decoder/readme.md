# Gameboy Printer Emulator Tile Decoder

This contains various decoder implementations you can use. For most people
the javascript decoder should be of most use.

## Javascript Tile Decoder

Written in javascript, this allows most users to copy over the arduino default
output into the web browser and render the image. This is the easiest

### V2 - 2020-08-16

This contains incremental improvements from the community to support multiple
photos in a single stream as well as to color the photos.

### V1 - 2017-04-6

First release. Kept here due to simpler coding structure which would be good for
understanding the basics. And also with less checks, it's good for developments.

## Octave/Matlab Tile Decoder

Rapha�l BOICHOT contributed a decoder written in octave/matlab that can parse
the raw packet mode output of gbp_emulator_v2.

### V1 - 2020-08-16

First release. To use it, copy over the output to `New_Format.txt` and run the
octave program `Arduino_Game_Boy_Printer_decoder_compression_palette.m`.