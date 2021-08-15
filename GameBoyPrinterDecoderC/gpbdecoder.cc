/*************************************************************************
 *
 * Gameboy Printer C Decoder
 * Part of GAMEBOY PRINTER EMULATION PROJECT V2 (Arduino)
 * Copyright (C) 2020 Brian Khuu
 *
 * PURPOSE: This program allows for decoding raw hex packets into bmp
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/types.h>

#include <stdlib.h>

#include "gameboy_printer_protocol.h"
#include "gbp_pkt.h"
#include "gbp_tiles.h"
#include "gbp_bmp.h"


/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "gpbdecoder"

/******************************************************************************/

static bool verbose_flag = false;
static bool display_flag = false;

/******************************************************************************/

// Input/Output file
const char * ifilename = NULL;
const char * ofilename = NULL;
FILE * ifilePtr = NULL;
char ofilenameBuf[255] = {0};
char ofilenameExt[50]  = {0};

/******************************************************************************/

// Pallet
const char * palletParameter = NULL;
uint32_t palletColor[4] = {0};

/******************************************************************************/

// Other Variables
uint8_t pktCounter = 0; // Dev Varible
gbp_pkt_t gbp_pktBuff = {GBP_REC_NONE, 0};
uint8_t gbp_pktbuff[GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE] = {0};
uint8_t gbp_pktbuffSize = 0;
gbp_pkt_tileAcc_t tileBuff = {0};
gbp_tile_t gbp_tiles = {0};
gbp_bmp_t  gbp_bmp = {0};

/******************************************************************************/

static void gbpdecoder_gotByte(const uint8_t byte);

/*******************************************************************************
 * Utilites
*******************************************************************************/

const char *gbpCommand_toStr(int val)
{
  switch (val)
  {
    case GBP_COMMAND_INIT    : return "INIT";
    case GBP_COMMAND_PRINT   : return "PRNT";
    case GBP_COMMAND_DATA    : return "DATA";
    case GBP_COMMAND_BREAK   : return "BREK";
    case GBP_COMMAND_INQUIRY : return "INQY";
    default: return "?";
  }
}

static void filenameExtractPathAndExtention(const char *fname,
                        char *pathBuff, int pathSize,
                        char *extBuff, int extSize)
{
    // Minimal Filename Extraction Of Path And Extention
    // Brian Khuu 2021
    int i = 0;
    int end = 0;
    int exti = 0;
    for (end = 0; fname[end] != '\0' ; end++)
    {
        if ((fname[end] == '/')||(fname[end] == '\\'))
          exti = 0;
        else if (fname[end] == '.')
          exti = end;
    }
    if (exti == 0)
        exti = end;

    // Copy PathName
    if (pathBuff)
    {
      for (i = 0; i < (pathSize-1) ; i++)
      {
          if (!(i < exti))
              break;
          pathBuff[i] = fname[i];
      }
      pathBuff[i] = '\0';
    }

    // Copy Extention
    if (extBuff)
    {
      for (i = 0; i < (extSize-1) ; i++)
      {
          if (!(i < (end-exti-1)))
            break;
          extBuff[i] = fname[exti + i + 1];
      }
      extBuff[i] = '\0';
    }
}

int palletColorParse(uint32_t *palletColor, const int palletColorSize, const char * parameterStr)
{
  // Parse web color hexes for pallet (e.g. `0xFFAD63, ...`) (e.g. `#FFAD63, ...`)
  // This was created for the cdecoder in https://github.com/mofosyne/arduino-gameboy-printer-emulator
  // https://gist.github.com/mofosyne/b1fc240b64c520c0bf3541a029e3dcc3
  // Brian Khuu 2021
  if (!parameterStr)
    return 0;
  int palletCounter = 0;
  int nibIndex = 0;
  uint32_t pallet = 0;
  char prevChar = 0;
  for ( ; (*(parameterStr)) != '\0' ; parameterStr++)
  {
    const char ch = *parameterStr;
    // Search for start of #XXXXXX, we are looking for 4 pallets
    if (nibIndex == 0)
    {
      if ((ch == '#') || ((prevChar == '0')&&(ch == 'x')))
      {
        pallet = 0;
        nibIndex = 3*2; // [R, G, B]
        prevChar = 0;
        continue;
      }
      prevChar = ch;
      continue;
    }
    nibIndex--;
    // Parse Nibble
    char nib = -1;
    if (('0' <= ch) && (ch <= '9'))
      nib = ch - '0';
    else if (('a' <= ch) && (ch <= 'f'))
      nib = ch - 'a' + 10;
    else if (('A' <= ch) && (ch <= 'F'))
      nib = ch - 'A' + 10;
    else
    {
      nibIndex = 0;
      palletColor[palletCounter] = pallet;
      palletCounter++;
      if (palletCounter >= palletColorSize)
        break;
      continue;
    }
    // Pallet
    pallet |= nib << (nibIndex*4);
    if (nibIndex == 0)
    {
      palletColor[palletCounter] = pallet;
      palletCounter++;
      if (palletCounter >= palletColorSize)
        break;
    }
  }
  return palletCounter;
}

/*******************************************************************************
 * Main Test Routine
*******************************************************************************/
void gpbdecoder_help(void)
{
  printf (
      "Usage: gpbdecoder [OPTION]...\n"
      "This program allows for decoding raw hex packets into bmp\n"
      "\n"
      "With no FILE, read standard input.\n"
      "\n"
      "-i, --input=FILE     input hexfile in ascii format\n"
      "-o, --output=OUTFILE output bmp filename\n"
      "-p, --pallet=PALLET  pallet color in web color format\n"
      "-h, --help           display this help and exit\n"
      "-d, --display        preview image via vt100 output\n"
      "-v, --verbose        verbose print\n"
      "\n"
      "Examples:\n"
      "  cat ./test/test.txt | gpbdecoder -p \"#ffffff#ffad63#833100#000000\" -o ./test/test.bmp    stdin based input, with a defined output filename\n"
      "-p \"#dbf4b4#abc396#7b9278#4c625a#FFFFFF00\" -i ./test/test.txt                              input file used. Output file has similar name to input file\n"
    );
}

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;

  int c;
  static struct option const long_options[] =
  {
    /* These options set a flag. */
    {"display", no_argument,       (int*)&display_flag, 1},
    {"verbose", no_argument,       (int*)&verbose_flag, 1},
    {"brief",   no_argument,       (int*)&verbose_flag, 0},
    /* These options don’t set a flag.
        We distinguish them by their indices. */
    {"input",   required_argument, NULL, 'i'},
    {"output",  required_argument, NULL, 'o'},
    {"pallet",  required_argument, NULL, 'p'},
    {"verbose", no_argument,       NULL, 'v'},
    {"help",    no_argument,       NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  while ((c = getopt_long (argc, argv, "o:i:p:vd", long_options, NULL))
         != -1)
  {
    switch (c)
    {
        case 'i':
          ifilename = optarg;
          break;

        case 'o':
          ofilename = optarg;
          break;

        case 'p':
          palletParameter = optarg;
          break;

        case 'v':
          verbose_flag = true;
          break;

        case 'd':
          display_flag = true;
          break;

        case 'h':
          gpbdecoder_help();
          return 0;
          break;
    }
  }

  /* Input File */
  if (ifilename)
  {
    ifilePtr = fopen(ifilename, "r+");
    if (ifilePtr == NULL)
    {
      printf("file not found\n");
      gpbdecoder_help();
      return 0;
    }
    printf("file input `%s' open\n", ifilename);
  }
  else
  {
    // Input file not found, use stdin
    ifilePtr = stdin;
    printf("file input stdin\n");
  }

  /* Output File */
  if (!ofilename)
  {
    // Default output filename if not defined
    if (ifilename)
    {
      // Use input filename (We will strip out any extention and add our own anyway)
      ofilename = ifilename;
    }
    else
    {
      ofilename = "gbpOut.bmp";
    }
  }
  filenameExtractPathAndExtention(ofilename, ofilenameBuf, sizeof(ofilenameBuf), ofilenameExt, sizeof(ofilenameExt));
  printf("file requested output `%s' (%s)\n", ofilenameBuf, ofilenameExt);

  /* Custom Pallet */
  if (palletColorParse(palletColor, sizeof(palletColor)/sizeof(palletColor[0]), palletParameter) == 0)
  {
    palletColor[0] = 0xFFFFFF;
    palletColor[1] = 0xAAAAAA;
    palletColor[2] = 0x555555;
    palletColor[3] = 0x000000;
  }
  printf("Pallet: 0x%06X, 0x%06X, 0x%06X, 0x%06X\n", palletColor[0], palletColor[1], palletColor[2], palletColor[3]);

  /****************************************************************************/
  gbp_pkt_init(&gbp_pktBuff);

  char ch = 0;
  bool skipLine = false;
  int  lowNibFound = 0;
  uint8_t byte = 0;
  unsigned int bytec = 0;
  while ((ch = fgetc(ifilePtr)) != EOF)
  {
    // Skip Comments
    if (ch == '/')
    {
      // Might be `//` or `/*`
      skipLine = true;
      continue;
    }
    else if (skipLine)
    {
      // Discarding line
      if (ch == '\n')
        skipLine = false;
      continue;
    }

    // Parse Nibble
    char nib = -1;
    if (('0' <= ch) && (ch <= '9'))
      nib = ch - '0';
    else if (('a' <= ch) && (ch <= 'f'))
      nib = ch - 'a' + 10;
    else if (('A' <= ch) && (ch <= 'F'))
      nib = ch - 'A' + 10;

    /* Parse As Byte */
    bool byteFound = false;
    // Hex Parse Edge Cases
    if (lowNibFound)
    {
      // '0x' found. Ignore
      if ((byte == 0) && (ch == 'x'))
        lowNibFound = false;
      // Not a hex digit pair. Ignore
      if (nib == -1)
        lowNibFound = false;
    }
    // Hex Byte Parsing
    if (nib != -1)
    {
      if (!lowNibFound)
      {
        lowNibFound = true;
        byte = nib << 4;
      }
      else
      {
        lowNibFound = false;
        byte |= nib << 0;
        byteFound = true;
      }
    }

    // Byte Was Found, decoding...
    if (byteFound)
    {
      bytec++;
      gbpdecoder_gotByte(byte);
    }
  }

  return 0;
}


void gbpdecoder_gotByte(const uint8_t byte)
{
  if (gbp_pkt_processByte(&gbp_pktBuff, byte, gbp_pktbuff, &gbp_pktbuffSize, sizeof(gbp_pktbuff)))
  {
    if (gbp_pktBuff.received == GBP_REC_GOT_PACKET)
    {
      pktCounter++;
      if (verbose_flag)
      {
        printf("// %s | compression: %1u, dlength: %3u, printerID: 0x%02X, status: %u | %d | ",
            gbpCommand_toStr(gbp_pktBuff.command),
            (unsigned) gbp_pktBuff.compression,
            (unsigned) gbp_pktBuff.dataLength,
            (unsigned) gbp_pktBuff.printerID,
            (unsigned) gbp_pktBuff.status,
            (unsigned) pktCounter
          );
        for (int i = 0 ; i < gbp_pktbuffSize ; i++)
        {
          printf("%02X ", gbp_pktbuff[i]);
        }
        printf("\r\n");
      }
      if (gbp_pktBuff.command == GBP_COMMAND_PRINT)
      {
        const bool cutPaper = ((gbp_pktbuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED]&0xF) != 0) ? true : false;  ///< if lower margin is zero, then new pic
        gbp_tiles_print(&gbp_tiles,
            gbp_pktbuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_SHEETS],
            gbp_pktbuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED],
            gbp_pktbuff[GBP_PRINT_INSTRUCT_INDEX_PALETTE_VALUE],
            gbp_pktbuff[GBP_PRINT_INSTRUCT_INDEX_PRINT_DENSITY]);

        if (display_flag)
        {
          if (cutPaper)
          {
            // Display Preview
            for (int j = 0; j < (GBP_TILE_PIXEL_HEIGHT * gbp_tiles.tileRowOffset); j++)
            {
              for (int i = 0; i < (GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE); i++)
              {
                const int pixel = 0b11 & (gbp_tiles.bmpLineBuffer[j][GBP_TILE_2BIT_LINEPACK_INDEX(i)] >> GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));
                int b = 0;
                switch (pixel)
                {
                  default:
                  case 3: b = 0; break;
                  case 2: b = 64; break;
                  case 1: b = 130; break;
                  case 0: b = 255; break;
                }
                printf("\x1B[48;2;%d;%d;%dm \x1B[0m", b, b, b);
              }
              printf("\r\n");
            }
            gbp_tiles_reset(&gbp_tiles);
          }
        }
        else
        {
          // Streaming BMP Writer
          // Dev Note: Done this way to allow for streaming writes to file without a large buffer

          // Open New File
          if (!gbp_bmp_isopen(&gbp_bmp))
          {
            gbp_bmp_open(&gbp_bmp, ofilenameBuf, GBP_TILE_PIXEL_WIDTH*GBP_TILES_PER_LINE);
          }

          // Write Decode Data Buffer Into BMP
          for (int j = 0; j < gbp_tiles.tileRowOffset; j++)
          {
            const long int tileHeightIncrement = GBP_TILE_PIXEL_HEIGHT*GBP_BMP_MAX_TILE_HEIGHT;
            gbp_bmp_add(&gbp_bmp, (const uint8_t *) &gbp_tiles.bmpLineBuffer[tileHeightIncrement*j][0], (GBP_TILE_PIXEL_WIDTH*GBP_TILES_PER_LINE), tileHeightIncrement, palletColor);
          }
          gbp_tiles_reset(&gbp_tiles); ///< Written to file, clear decoded tile line buffer

          // Print finished and cut requested
          if (cutPaper)
          {
            gbp_bmp_render(&gbp_bmp);
          }
        }
      }
    }
    else
    {
      // Support compression payload
      while (gbp_pkt_decompressor(&gbp_pktBuff, gbp_pktbuff, gbp_pktbuffSize, &tileBuff))
      {
        if (gbp_pkt_tileAccu_tileReadyCheck(&tileBuff))
        {
          // Got tile
#if 0     // Output Tile As Hex For Debugging purpose
          for (int i = 0 ; i < GBP_TILE_SIZE_IN_BYTE ; i++)
          {
            printf("%02X ", tileBuff.tile[i]);
          }
          printf("\r\n");
#endif
          if (gbp_tiles_line_decoder(&gbp_tiles, tileBuff.tile))
          {
            // Line Obtained
#if 0       // Per Line Decoded (Pre Pallet Harmonisation)
            for (int j = 0; j < GBP_TILE_PIXEL_HEIGHT; j++)
            {
              for (int i = 0; i < (GBP_TILE_PIXEL_WIDTH * GBP_TILES_PER_LINE); i++)
              {
                int pixel = 0b11 & (gbp_tiles.bmpLineBuffer[j+(gbp_tiles.tileRowOffset-1)*8][GBP_TILE_2BIT_LINEPACK_INDEX(i)] >> GBP_TILE_2BIT_LINEPACK_BITOFFSET(i));;
                int b = 0;
                switch (pixel)
                {
                  case 0: b = 0; break;
                  case 1: b = 64; break;
                  case 2: b = 130; break;
                  case 3: b = 255; break;
                }
                printf("\x1B[48;2;%d;%d;%dm \x1B[0m", b, b, b);
              }
              printf("\r\n");
            }
#endif
          }
        }
      }
    }
  }
}

