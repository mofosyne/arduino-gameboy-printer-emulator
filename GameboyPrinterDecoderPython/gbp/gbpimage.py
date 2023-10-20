# Copyright (C) 2023 Teemu Ikonen
from . import gbpparser

TILE_PIXEL_WIDTH = 8
TILE_PIXEL_HEIGHT = 8
TILES_PER_LINE = 20  # Gameboy Printer Tile Constant


def decodePackets(packets, verbose=False):
    imageStream = gbpparser.get_imagedata_stream(packets)
    decompressed = gbpparser.decompress_data_stream(imageStream)
    if verbose:
        print(f'{len(decompressed)} image packets decompressed')
        for p in imageStream:
            print(f'{gbpparser.command_to_str(p.command)} {len(p.data)}B')

    withDecodedCommands = gbpparser.decode_print_commands(decompressed)
    if verbose:
        for p in imageStream:
            if p.command == gbpparser.COMMAND_PRINT:
                print(f'Print data {p.data}')

    withHarmonizedPalette = gbpparser.harmonize_palettes(withDecodedCommands)
    return withHarmonizedPalette


def decodeHexDumpToPackets(hexdata, verbose=False):

    # Image data consists of DATA packets and a PRINT packet

    rawBytes = gbpparser.to_byte_array(hexdata)
    if verbose:
        print(f'{len(hexdata)} data bytes')

    packets = gbpparser.parse_packets(rawBytes)
    if verbose:
        print(f'{len(packets)} packets found')
        for p in packets:
            print(f'{gbpparser.command_to_str(p.command)} compressed: {p.hasCompression}  length: {p.dataLength}B checksumOk: {p.checksumOK}')

    return decodePackets(packets, verbose)


# Tiles is 8x8 pixels encoded as 16 bytes. 2 bits per pixel, coded in a weird way. Details here:
# https://www.huderlem.com/demos/gameboy2bpp.html

def dataTopixels(data):

    pixels = []
    for i in range(0, len(data), 2):
        bh = data[i+1]
        bl = data[i]
        pixels.extend(gbpparser.decode_2BPP(bh, bl))
    return pixels


def decode2BPPtoTiles(imageData):
    tiles = []
    palette = None
    for packet in imageData:
        if packet.command == gbpparser.COMMAND_DATA:
            row = []
            # each tile is 16 bytes
            for i in range(0, len(packet.data), 16):
                tile = dataTopixels(packet.data[i:i+16])
                tiles.append(tile)
        elif packet.command == gbpparser.COMMAND_PRINT:
            palette = packet.data['paletteData']

    return (tiles, palette)


def decodeTilesToPixels(tiles, palette=[255, 85, 170,  0], tiles_per_line=TILES_PER_LINE):
    pixels_per_tile = TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT
    pixels = [palette[0]] * pixels_per_tile * len(tiles)
    stride = tiles_per_line * TILE_PIXEL_WIDTH

    for i in range(0, len(tiles), tiles_per_line):
        lineidx = pixels_per_tile * i
        colidx = 0
        for tile in tiles[i:i+tiles_per_line]:
            # Map tile to a flat pixel array with palette
            for (x, y) in [(x, y) for x in range(0, 8) for y in range(0, 8)]:
                p = palette[tile[y * TILE_PIXEL_WIDTH + x]]
                pixels[lineidx + y * stride + colidx + x] = p
            colidx += TILE_PIXEL_WIDTH

    width = TILE_PIXEL_WIDTH * tiles_per_line
    height = int(len(tiles) / tiles_per_line * TILE_PIXEL_HEIGHT)
    return (pixels, (width, height))
