#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <assert.h>

#include "gbp_tiles.h"
#include "gbp_bmp.h"
#include "./image/bmp_FixedWidthStream.h"

bool gbp_bmp_isopen(gbp_bmp_t * gbp_bmp)
{
    return (gbp_bmp->f != 0) ? true : false;
}

void gbp_bmp_open(gbp_bmp_t * gbp_bmp, const char *outputFilename, const uint16_t fixed_width_size)
{
    if (gbp_bmp->f != 0)
    {
        fclose(gbp_bmp->f);
        gbp_bmp->f = 0;
    }

    // Open file
    char filenameBuff[400] = {0};
    snprintf(filenameBuff, sizeof(filenameBuff), "%s%X.bmp", outputFilename, gbp_bmp->fileCounter);
    gbp_bmp->f = fopen(filenameBuff, "wb");

    // Skip over bmp header...
    fseek(gbp_bmp->f, BMP_PIXEL_START_OFFSET, SEEK_SET);

    // Update
    gbp_bmp->bmpSizeWidth  = fixed_width_size;
    gbp_bmp->bmpSizeHeight = 0;
    gbp_bmp->fileCounter++;
}

void gbp_bmp_add(gbp_bmp_t * gbp_bmp, const uint8_t * bmpLineBuffer, const uint16_t sizex, const uint16_t sizey, const uint32_t palletColor[4])
{
    // Fixed width
    if (sizex != gbp_bmp->bmpSizeWidth)
        return;

    for (uint16_t y = 0; y < sizey; y++)
    {
        for (uint16_t x = 0; x < sizex; x++)
        {
            const int pixel = 0b11 & (bmpLineBuffer[(y * GBP_TILE_2BIT_LINEPACK_ROWSIZE_B(sizex)) + GBP_TILE_2BIT_LINEPACK_INDEX(x)] >> GBP_TILE_2BIT_LINEPACK_BITOFFSET(x));
            const unsigned long encodedColor = palletColor[pixel & 0b11];
            //printf("bmp_set %d, %d\r\n", x, y);
            bmp_set(gbp_bmp->bmpBuffer, sizex, x, y, encodedColor);
        }
    }

    fwrite(gbp_bmp->bmpBuffer, BMP_PIXEL_BUFF_SIZE(sizex, sizey), 1, gbp_bmp->f);
    gbp_bmp->bmpSizeHeight += sizey;
}

void gbp_bmp_render(gbp_bmp_t * gbp_bmp)
{
    // Rewind and write header with the now known image size
    fseek(gbp_bmp->f, 0, SEEK_SET);
    bmp_header(gbp_bmp->bmpBuffer, gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight);
    fwrite(gbp_bmp->bmpBuffer, BMP_PIXEL_START_OFFSET, 1, gbp_bmp->f);

    // Close File
    fclose(gbp_bmp->f);
    gbp_bmp->f = 0;
}
