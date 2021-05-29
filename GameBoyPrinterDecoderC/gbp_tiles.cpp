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
#include "gbp_tiles.h"

static void gbp_tiles_toBuff(
                        uint8_t *buff,
                        const int buffSize,
                        const int buffTileCount,
                        const int tileOffset,
                        const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE])
{
    // This converts tile data to a bitmap buffer
    // The bitmap buffer has enough space to contain multiple decoded tiles
    // And each area is written to by x tile offset

    // Guard
    if (buffSize < (buffTileCount * GBP_TILE_PIXEL_HEIGHT * GBP_TILE_PIXEL_WIDTH))
        return;

    // Tile Decoder
    for (uint8_t j = 0; j < GBP_TILE_PIXEL_HEIGHT; j++)
    {
        for (uint8_t i = 0; i < GBP_TILE_PIXEL_WIDTH; i++)
        {
            const int offset    = tileOffset    * GBP_TILE_PIXEL_WIDTH;
            const int widthSize = buffTileCount * GBP_TILE_PIXEL_WIDTH;
            const uint8_t hiBit = (uint8_t)((tileBuff[j*2 + 1] >> (7 - i)) & 1);
            const uint8_t loBit = (uint8_t)((tileBuff[j*2    ] >> (7 - i)) & 1);
            const uint8_t value = (uint8_t)((hiBit << 1) | loBit); // 0-3
            buff[j * widthSize + offset + i] = value;
        }
    }
}

bool gbp_tiles_line_decoder(gbp_tile_t *gbp_tiles, const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE])
{
    gbp_tiles_toBuff(
                        (uint8_t *)gbp_tiles->bmpLineBuffer,
                        GBP_TILE_PIXEL_HEIGHT * GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE,
                        GBP_TILES_PER_LINE,
                        gbp_tiles->tileOffset,
                        tileBuff);
    gbp_tiles->tileOffset++;
    if (gbp_tiles->tileOffset >= GBP_TILES_PER_LINE)
    {
        // Enough tiles decoded to output a fully decoded line
        gbp_tiles->tileOffset = 0;
        return true;
    }

    // Tile Decoded, but not enough to make a line
    return false;
}
