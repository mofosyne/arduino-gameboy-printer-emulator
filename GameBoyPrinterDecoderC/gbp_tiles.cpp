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
                        const int tileLineOffset,
                        const int tileRowOffset,
                        const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE])
{
    // This converts tile data to a bitmap buffer
    // The bitmap buffer has enough space to contain multiple decoded tiles
    // And each area is written to by x tile offset

    // Guard
    if (buffSize < (buffTileCount * GBP_TILE_PIXEL_HEIGHT * GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(GBP_TILE_PIXEL_WIDTH)))
        return;

    const int lineWidthSize = GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(buffTileCount * GBP_TILE_PIXEL_WIDTH);
    const int rowHeightSize = lineWidthSize * GBP_TILE_PIXEL_HEIGHT;

    // Tile Decoder
    for (int j = 0; j < GBP_TILE_PIXEL_HEIGHT; j++)
    {
        for (int i = 0; i < GBP_TILE_PIXEL_WIDTH; i++)
        {
            const int offset    = tileLineOffset * GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(GBP_TILE_PIXEL_WIDTH);
            const uint8_t hiBit = (uint8_t)((tileBuff[j*2 + 1] >> (7 - i)) & 1);
            const uint8_t loBit = (uint8_t)((tileBuff[j*2    ] >> (7 - i)) & 1);
            const uint8_t value = (uint8_t)((hiBit << 1) | loBit); // 0-3
            const unsigned int indx = (tileRowOffset * rowHeightSize) + (j * lineWidthSize) + offset + GBP_TILE_2BIT_LINEPACK_INDEX(i);
            buff[indx] &= ~(0b11 << GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));
            buff[indx] |= value << GBP_TILE_2BIT_LINEPACK_BITOFFSET(i);
        }
    }
}

bool gbp_tiles_line_decoder(gbp_tile_t *gbp_tiles, const uint8_t tileBuff[GBP_TILE_SIZE_IN_BYTE])
{
    gbp_tiles_toBuff(
                        (uint8_t *)gbp_tiles->bmpLineBuffer,
                        GBP_TILE_PIXEL_HEIGHT * GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE,
                        GBP_TILES_PER_LINE,
                        gbp_tiles->tileLineOffset,
                        gbp_tiles->tileRowOffset,
                        tileBuff);
    gbp_tiles->tileLineOffset++;
    if (gbp_tiles->tileLineOffset >= GBP_TILES_PER_LINE)
    {
        // Enough tiles decoded to output a fully decoded line
        gbp_tiles->tileLineOffset = 0;
        gbp_tiles->tileRowOffset++;
        return true;
    }

    // Tile Decoded, but not enough to make a line
    return false;
}

/*****************************************************************************/

void gbp_tiles_reset(gbp_tile_t *gbp_tiles)
{
    (void)gbp_tiles;
    gbp_tiles->tileLineOffset = 0;
    gbp_tiles->tileRowOffset  = 0;
    gbp_tiles->tileRowOffsetHarmonised =0;
}

void gbp_tiles_print(gbp_tile_t *gbp_tiles, uint8_t sheet, uint8_t linefeed, uint8_t pallet, uint8_t density)
{
    (void)gbp_tiles;
    (void)sheet;
    (void)linefeed;
    (void)pallet;
    (void)density;

    /* Harmonise Pallete */
    // Ref: https://github.com/Raphael-Boichot/The-Arduino-SD-Game-Boy-Printer#some-technical-facts
    // Palette 0x00 has the same effect than palette 0xE4 (the mainly encountered palette in games)
    uint8_t tonePallet[GBP_TILE_MAX_TONES] = {0};
    pallet = (pallet == 0x00) ? 0xE4 : pallet;
    tonePallet[0] = ((pallet >> 0) & 0b11);
    tonePallet[1] = ((pallet >> 2) & 0b11);
    tonePallet[2] = ((pallet >> 4) & 0b11);
    tonePallet[3] = ((pallet >> 6) & 0b11);
    const int startH = GBP_TILE_PIXEL_HEIGHT * gbp_tiles->tileRowOffsetHarmonised;
    const int endH   = GBP_TILE_PIXEL_HEIGHT * gbp_tiles->tileRowOffset;

    if (startH > endH)
        return;

    for (int j = startH; j < endH; j++)
    {
        for (int i = 0; i < GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE; i++)
        {
            const uint8_t pixel = 0b11 & (gbp_tiles->bmpLineBuffer[j][GBP_TILE_2BIT_LINEPACK_INDEX(i)] >> GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));
            const uint8_t harmonised = tonePallet[pixel & 0b11];
            gbp_tiles->bmpLineBuffer[j][GBP_TILE_2BIT_LINEPACK_INDEX(i)] &= ~(0b11 << GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));
            gbp_tiles->bmpLineBuffer[j][GBP_TILE_2BIT_LINEPACK_INDEX(i)] |= (harmonised << GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));
        }
    }
    gbp_tiles->tileRowOffsetHarmonised = gbp_tiles->tileRowOffset;
}


