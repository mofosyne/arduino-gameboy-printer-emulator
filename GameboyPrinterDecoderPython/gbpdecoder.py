# Gameboy printer data to image decoder
# Copyright (C) 2023 Teemu Ikonen

import argparse
import os
import re

from gbp import gbpimage

verbose_debug = False


def stripComments(hexdata):
    # Removes comments like //.. and /* ... */
    p = re.compile(r'^\/\*.*\*\/|^\/\/.*$', re.MULTILINE)
    return re.sub(p, '', hexdata)


# Write out png files
def savePNG(pixels, w, h, outfilebase):

    def chunker(seq, size):
        return (seq[pos:pos + size] for pos in range(0, len(seq), size))

    from PIL import Image, ImageDraw
    import numpy as np
    pixels = list(chunker(pixels, w))
    raw = np.array(pixels, dtype=np.uint8)
    out_img = Image.fromarray(raw)
    outfile = outfilebase + ".png"
    out_img.save(outfile)
    print("Wrote " + outfile)

    out_img = out_img.resize((w*2, h*2), Image.Resampling.LANCZOS)
    outfile = outfilebase + "-2x.png"
    out_img.save(outfile)
    print("Wrote " + outfile)


def hexToImage(hexdata):
    # Decode hexadecimal data to packets
    bpp = gbpimage.decodeHexDumpToPackets(hexdata, verbose_debug)
    for p in bpp:
        if not p.checksumOK:
            print(
                f'WARNING: Command {p.command}. Checksum {hex(p.checksum)} does not match data.')

    (tiles, palette) = gbpimage.decode2BPPtoTiles(bpp)
    if verbose_debug:
        print(f'{len(tiles)} Tiles. Palette {palette}')

    # Color palette is from GB palette index to grayscale. Usually 0 -> white, 1 and 3 -> black.
    palette = [255, 85, 170,  0]
    (pixels, (w, h)) = gbpimage.decodeTilesToPixels(
        tiles, palette=palette)
    if verbose_debug:
        print(f'{w} x {h} (w x h) {len(pixels)} pixels')

    return (pixels, (w, h))


def main():

    parser = argparse.ArgumentParser(
        description='GameBoy Printer Data Decoder')
    parser.add_argument('--verbose', action='store_true', help='verbose mode')
    parser.add_argument('-i', metavar='FILE', type=str,
                        help='Input hexfile in ASCII format')
    args = parser.parse_args()

    filename = 'out'
    hexdata = ''
    global verbose_debug
    verbose_debug = args.verbose

    if args.i:
        filename = args.i
        if verbose_debug:
            print("Reading", filename)
        hexdata = open(filename).read()
    else:
        import sys
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            hexdata += line

    hexdata = stripComments(hexdata)
    (pixels, (w, h)) = hexToImage(hexdata)

    if len(pixels) == w*h and len(pixels) > 0:
        savePNG(pixels, w, h, os.path.splitext(filename)[0])
    else:
        print("No image data!")


if __name__ == '__main__':
    main()
