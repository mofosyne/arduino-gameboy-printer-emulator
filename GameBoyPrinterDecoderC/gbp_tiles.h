/*************************************************************************
 *
 * Gameboy Printer Tile Decoder
 * Part of GAMEBOY PRINTER EMULATION PROJECT V2 (Arduino)
 * Copyright (C) 2020 Brian Khuu
 *
 * PURPOSE: This module focus on decoder gameboy printer tiles to bmp
 * LICENCE:
 *   This file is part of Arduino Gameboy Printer Emulator.
 *
 *   Arduino Gameboy Printer Emulator is free software:
 *   you can redistribute it and/or modify it under the terms of the
 *   GNU General Public License as published by the Free Software Foundation,
 *   either version 3 of the License, or (at your option) any later version.
 *
 *   Arduino Gameboy Printer Emulator is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Arduino Gameboy Printer Emulator.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include <stdbool.h> // bool
#include "gameboy_printer_protocol.h"

/*
    Dev Note: From https://gbdev.io/pandocs/Gameboy_Printer.html
    GBP has about 8 KiB of RAM to buffer incoming graphics data.
    Those 8 KiB allow a maximum bitmap area of 160*200 (8192/160*4) pixels between prints.

    ```
    buffSize = 8*1024;
    bitsPerPixel = 2;
    linePixelCount = 8*20;
    linebitcount = bitsPerPixel * linePixelCount;
    bytesPerLine = linebitcount / 8;
    maxRows = buffSize / (bytesPerLine * 8)
        maxRows = 25.6
    ```
*/

// IMAGE DEFINITION
#define GBP_TILE_PIXEL_WIDTH  8
#define GBP_TILE_PIXEL_HEIGHT 8
#define GBP_TILES_PER_LINE    20
#define GBP_TILES_PER_ROW     26  // Number of supported lines between print commands (26 tile row height is based on the 8KiB of a real gbp printer)
#define GBP_TILE_MAX_TONES    4   // 2bits per pixel

// 2B per pixel packing
#define GBP_TILE_2BIT_LINEPACK_INDEX(x) (x/4)
#define GBP_TILE_2BIT_LINEPACK_BITOFFSET(x) (2*(x%4))
#define GBP_TILE_2BIT_LINEPACK_IN_BYTE_COUNT (4) ///< 4 2bit pixel in 8bit byte
#define GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(byteCount) (byteCount/4) ///< Row sized when 2bit packed is reduced by factor of 4

typedef struct
{
    // This is the tile to bmp decoder
    uint16_t tileLineOffset;
    uint16_t tileRowOffset;
    uint16_t tileRowOffsetHarmonised;

    // Each array entry represents a decoded 2bit pixel
    uint8_t bmpLineBuffer[GBP_TILE_PIXEL_HEIGHT * GBP_TILES_PER_ROW][GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE)];
} gbp_tile_t;

bool gbp_tiles_line_decoder(gbp_tile_t *gbp_tiles, const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE]);
void gbp_tiles_reset(gbp_tile_t *gbp_tiles);
void gbp_tiles_print(gbp_tile_t *gbp_tiles, uint8_t sheet, uint8_t linefeed, uint8_t pallet, uint8_t density);