#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "./image/bmp_FixedWidthStream.h"


// Image Rendering
#define GBP_BMP_MAX_TILE_HEIGHT 1
#define GBP_BMP_WIDTH  (GBP_TILE_PIXEL_WIDTH  * GBP_TILES_PER_LINE)
#define GBP_BMP_HEIGHT (GBP_TILE_PIXEL_HEIGHT * GBP_BMP_MAX_TILE_HEIGHT)

typedef struct
{
    FILE *f;
    int fileCounter;
    uint16_t bmpSizeWidth;  // x
    uint16_t bmpSizeHeight; // y
    unsigned char bmpBuffer[BMP_PIXEL_BUFF_SIZE(GBP_BMP_WIDTH, GBP_BMP_HEIGHT)];
} gbp_bmp_t;


bool gbp_bmp_isopen(gbp_bmp_t * gbp_bmp);
void gbp_bmp_open(gbp_bmp_t * gbp_bmp, const char *outputFilename, const uint16_t fixed_width_size);
void gbp_bmp_add(gbp_bmp_t * gbp_bmp, const uint8_t * bmpLineBuffer, const uint16_t sizex, const uint16_t sizey, const uint32_t palletColor[4]);
void gbp_bmp_render(gbp_bmp_t * gbp_bmp);
