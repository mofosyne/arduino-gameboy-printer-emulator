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

// IMAGE DEFINITION
#define GBP_TILE_PIXEL_WIDTH  8
#define GBP_TILE_PIXEL_HEIGHT 8
#define GBP_TILES_PER_LINE    20
#define GBP_IMG_WIDTH         160 //= GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE
#define GBP_IMG_HEIGHT        16
#define GBP_BYTES_PER_LINE    GBP_IMG_WIDTH * GBP_IMG_HEIGHT / 8

typedef struct
{
    // This is the tile to bmp decoder
    unsigned char tileOffset;
    uint8_t bmpLineBuffer[GBP_TILE_PIXEL_HEIGHT][GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE] = {0};
} gbp_tile_t;

bool gbp_tiles_line_decoder(gbp_tile_t *gbp_tiles, const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE]);
