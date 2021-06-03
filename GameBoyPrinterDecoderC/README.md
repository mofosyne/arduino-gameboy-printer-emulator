
```
Usage: gpbdecoder [OPTION]...
This program allows for decoding raw hex packets into bmp

With no FILE, read standard input.

-i, --input=FILE     input hexfile in ascii format
-o, --output=OUTFILE output bmp filename
-p, --pallet=PALLET  pallet color in web color format
-h, --help           display this help and exit
-d, --display        preview image via vt100 output
-v, --verbose        verbose print

Examples:
  cat ./test/test.txt | gpbdecoder -p "#ffffff#ffad63#833100#000000" -o ./test/test.bmp    stdin based input, with a defined output filename
-p "#dbf4b4#abc396#7b9278#4c625a#FFFFFF00" -i ./test/test.txt                              input file used. Output file has similar name to input file
```

![](./test/test0.bmp)

![](./test/test1.bmp)


## Building

Run make to build gpbdecoder

```
make
```


## Test

Run these make commands to test

```
make testdisplay
make test
```