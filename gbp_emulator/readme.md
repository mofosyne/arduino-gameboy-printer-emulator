# Gameboy Printer Emulator

## V2 - 2020-08-16

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

## V1 - 2017-04-6

First release. Kept here due to simpler coding structure which would be good for
understanding the basics. Also to ensure support for anyone else who is still
using this output for their own apps.