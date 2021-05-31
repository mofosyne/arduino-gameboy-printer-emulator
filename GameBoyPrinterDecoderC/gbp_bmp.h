#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "./image/bmp.h"


// Image Rendering
#define GBP_BMP_MAX_TILE_HEIGHT 100
#define GBP_BMP_WIDTH  (GBP_TILE_PIXEL_WIDTH  * GBP_TILES_PER_LINE)
#define GBP_BMP_HEIGHT (GBP_TILE_PIXEL_HEIGHT * GBP_BMP_MAX_TILE_HEIGHT)

typedef struct
{
    unsigned int bmpSizeWidth;  // x
    unsigned int bmpSizeHeight; // y
    char bmpBuffer[BMP_SIZE(GBP_BMP_WIDTH, GBP_BMP_HEIGHT)];
} gbp_bmp_t;
