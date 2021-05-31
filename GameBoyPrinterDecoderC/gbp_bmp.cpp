#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <assert.h>

#include "gbp_tiles.h"
#include "gbp_bmp.h"
#include "./image/bmp.h"


void gbp_bmp_reset(gbp_bmp_t * gbp_bmp)
{
    gbp_bmp->bmpSizeWidth  = GBP_BMP_WIDTH; // Fixed
    gbp_bmp->bmpSizeHeight = 0;
}


void gbp_bmp_addLine(gbp_bmp_t * gbp_bmp, uint8_t * bmpLineBuffer, uint8_t bmpLineBufferCount)
{
    for (uint8_t i = 0; i < GBP_BMP_WIDTH; i++)
    {
        int r, g, b;
        if (!(i < bmpLineBufferCount))
        {
            r = g = b = 0;
        }
        else
        {
            const int pixel = bmpLineBuffer[i];
            switch (pixel & 0b11)
            {
                default:
                case 3: r = g = b =   0; break;
                case 2: r = g = b =  64; break;
                case 1: r = g = b = 130; break;
                case 0: r = g = b = 255; break;
            };
        }
        bmp_set(gbp_bmp->bmpBuffer, gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight, bmp_encode(r, g, b));
    }
}


void gbp_bmp_complete(gbp_bmp_t * gbp_bmp)
{
    FILE *f;
    long x, y;
    bmp_init(gbp_bmp->bmpBuffer, gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight);

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            float r = y / (float)HEIGHT;
            float g = x / (float)WIDTH;
            float b = 1.0f;
            bmp_set(bmp, x, y, bmp_encode(r, g, b));
        }
    }

}
