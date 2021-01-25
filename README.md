# Arduino Gameboy Printer Emulator (Release V2) (2020-08-16)

**This is copied from V3 Readme, but edited to focus on V2 steps**

Main project website located at [https://mofosyne.github.io/arduino-gameboy-printer-emulator/](https://mofosyne.github.io/arduino-gameboy-printer-emulator/)

## V2 Version Description

### V2 gbp_emulator

Second major release is a total rewrite focusing on extensibility and portability
of the code. Aside from the `.ino` file, all other files is written in pure C
code with no Arduino dependencies. You can run a test program in your computer
in the `./test/` folder to test both the serial io and packet parser.

Historically the test folder was used during coding to ensure that what was
written matches the serial capture from `gbp_sniffer` in the research folder.

Press `?` to open up menu of options.

New feature of this arduino code:

* Run Length Compression decoder (e.g. Pokemon Trading Card Game) (Disabled due to not working on arduino nano)
* Doublespeed support (e.g. Pokemon Trading Card Game)
* Correct status byte response (e.g. Pokemon Yellow)
* Raw packet output mode (Good for technical research purpose)

This means larger support for a wide variety of games with more advance printer
drivers e.g. pokemon trading card game.

### V2 gbp_decoder

This contains incremental improvements from the community to support multiple
photos in a single stream as well as to color the photos.\

## Telegram Gameboy Camera Chatroom

Got telegram instant messaging and have some questions or need any advice, or just want to share? Invite link below:

**[https://t.me/gameboycamera](https://t.me/gameboycamera)**


## About this project

Code to emulate a gameboy printer via the gameboy link cable and an arduino module

![](./sample_image/gameboy_printer_emulator.png)

* [Blog Post](http://briankhuu.com/projects/gameboy_camera_arduino/gameboy_camera_arduino.html)

Goal is to provide an easy way for people to quickly setup and download the images from their gameboy to their computer before the battery of these gameboy cameras dies of old age.

I hope there will be a project to collate these gameboy images somewhere.

## Quick Start

### Construct the Arduino Gameboy Printer Emulator

Use an arduino nano and wire the gameboy link cable as shown below.
If you can fit the gameboy camera to gameboy advance etc... you may need a
differen pinout reference. But the wiring should be similar.

* [Pinout Reference](http://www.hardwarebook.info/Game_Boy_Link)


### Pinout Diagram

Thanks to West McGowan (twitter: @imwestm) who was able to replicate this project on his Arduino Nano plus Gameboy Color and helpfully submitted a handy picture of how to wire this project up. You can find his tutorial in [here](https://westm.co.uk/arduino-game-boy-printer-emulator/)

![](GBP_Emu_Micro_pinout_West_McGowan.webp.png)

### General Pinout

```
Gameboy Original/Color Link Cable Pinout
 ___________
|  6  4  2  |
 \_5__3__1_/   (at cable)
```

| Arduino Pin | Gameboy Link Pin                 |
|-------------|----------------------------------|
|  unused     | Pin 1 : 5.0V                     |
|  D4         | Pin 2 : Serial OUTPUT            |
|  D3         | Pin 3 : Serial INPUT             |
|  unused     | Pin 4 : Serial Data              |
|  D2         | Pin 5 : Serial Clock (Interrupt) |
|  GND        | Pin 6 : GND (Attach to GND Pin)  |

### Programming the emulator

* Arduino Project File: `./gbp_emulator_v2/gpb_emulator_v2.ino`
* Baud 115200 baud

Next download `./gbp_emulator_v2/gpb_emulator_v2.ino` to your arduino nano.
After that, open the serial console and set the baud rate to 115200 baud.

### Download the image

Press the download button in your gameboy. The emulator will automatically start to download and dump the data as a string of hex in the console display.

After the download has complete. Copy the content of the console to the javascript decoder in `./jsdecoderV2/gameboy_printer_js_decoder.html`. Press click to render button.

One you done that, your image will show up below. You can then right click on the image to save it to your computer. Or you can click upload to imgur to upload it to the web in public, so you can share it. (Feel free to share with me at mofosyne@gmail.com).

You are all done!


## My Face In BW

![](./sample_image/bk_portrait.png)

----

# Credits / Other Resources

## Resources Referenced

* GameBoy PROGRAMMING MANUAL Version 1.0 DMG-06-4216-001-A Released 11/09/1999
    - Is the original programming manual from nintendo. Has section on gameboy printer. Copy included in research folder.
* [http://www.huderlem.com/demos/gameboy2bpp.html](http://www.huderlem.com/demos/gameboy2bpp.html) Part of the js decoder code is based on the gameboy tile decoder tutorial
* [https://github.com/gism/GBcamera-ImageSaver](https://github.com/gism/GBcamera-ImageSaver) - Eventally found that someone else has already tackled the same project.
    - However was not able to run this sketch, and his python did not run. So there may have been some code rot.
    - Nevertheless I was able to get some ideas on using an ISR to capture the bits fast enough.
    - I liked how he outputs in .bmp format
* [http://gbdev.gg8.se/wiki/articles/Gameboy_Printer](http://gbdev.gg8.se/wiki/articles/Gameboy_Printer) - Main gb documentation on gb protocol, great for the inital investigation.
* [http://furrtek.free.fr/?a=gbprinter&i=2](http://furrtek.free.fr/?a=gbprinter&i=2) - Previous guy who was able to print to a gameboy printer
* [http://playground.arduino.cc/Main/Printf](http://playground.arduino.cc/Main/Printf) - printf in arduino
* [https://www.mikrocontroller.net/attachment/34801/gb-printer.txt](https://www.mikrocontroller.net/attachment/34801/gb-printer.txt)
    - Backup if above link is dead [here](./ext_doc/gb-printer.txt)
    - Most detailed writeup on the protocol I found online.
* [http://gbdev.gg8.se/wiki/articles/Serial_Data_Transfer_(Link_Cable)](http://gbdev.gg8.se/wiki/articles/Serial_Data_Transfer_(Link_Cable))
* [https://github.com/avivace/awesome-gbdev](https://github.com/avivace/awesome-gbdev) Collections of gameboy development resources
* [https://shonumi.github.io/articles/art2.html](https://shonumi.github.io/articles/art2.html) An in-depth technical document about the printer hardware, the communication protocol and the usual routine that games used for implementing the print feature.

## Contributors / Thanks

* @BjornB2 : For adding improvements in downloading images for jsdecoder folder https://github.com/mofosyne/arduino-gameboy-printer-emulator/pull/15
For adding color palette dropdown https://github.com/mofosyne/arduino-gameboy-printer-emulator/pull/18

* @virtuaCode : For helping to fix rendering issues with the jsdecoder https://github.com/mofosyne/arduino-gameboy-printer-emulator/pull/9

* @HerrZatacke : For adding the feature to render a separate image for each received image in jsdecoder https://github.com/mofosyne/arduino-gameboy-printer-emulator/pull/19

* @imwestm : West McGowan for submitting a handy picture of how to wire this project up as well as feedback to improve the instructions in this readme.

* Raphaël BOICHOT : For assistance with capturing gameboy communications and timing for support with gameboy printer fast mode and compression. Assisting in the support of more games. Also contributed a matlab/octave decoder implementation.

* @crizzlycruz (@23kpixels) : For adding support to js decoder for zero margin multi prints