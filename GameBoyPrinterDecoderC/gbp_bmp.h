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
    int fileCounter;
    uint16_t bmpSizeWidth;  // x
    uint16_t bmpSizeHeight; // y
    char bmpBuffer[BMP_SIZE(GBP_BMP_WIDTH, GBP_BMP_HEIGHT)];
} gbp_bmp_t;

void gbp_bmp_render(gbp_bmp_t * gbp_bmp, const char *outputFilename, uint8_t * bmpLineBuffer, uint16_t sizex, uint16_t sizey);

void gbp_bmp_rendertest(gbp_bmp_t * gbp_bmp, uint16_t sizex, uint16_t sizey);
