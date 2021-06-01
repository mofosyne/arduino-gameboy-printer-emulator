#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <assert.h>

#include "gbp_tiles.h"
#include "gbp_bmp.h"
#include "./image/bmp.h"

#if 0
void gbp_bmp_reset(gbp_bmp_t * gbp_bmp)
{
    gbp_bmp->bmpSizeWidth  = GBP_BMP_WIDTH; // Fixed
    gbp_bmp->bmpSizeHeight = 0;
}
#endif

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

void gbp_bmp_render(gbp_bmp_t * gbp_bmp, const char *outputFilename,  uint8_t * bmpLineBuffer, uint16_t sizex, uint16_t sizey)
{
    char filenameBuff[400] = {0};
    char filenamePath[200] = {0};
    FILE *f;
    gbp_bmp->bmpSizeWidth  = sizex;
    gbp_bmp->bmpSizeHeight = sizey;

    printf("--> %s\r\n", outputFilename);
    filenameExtractPathAndExtention(outputFilename, filenamePath, sizeof(filenamePath), NULL, 0);
    snprintf(filenameBuff, sizeof(filenameBuff), "%s%X.bmp", filenamePath, gbp_bmp->fileCounter%200);
    gbp_bmp->fileCounter++;

    bmp_init(gbp_bmp->bmpBuffer, gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight);
    for (uint16_t y = 0; y < sizey; y++)
    {
        for (uint16_t x = 0; x < sizex; x++)
        {
            const int pixel = bmpLineBuffer[y*sizex + x];
            int b = 0;
            switch (pixel)
            {
                case 3: b = 0; break;
                case 2: b = 64; break;
                case 1: b = 130; break;
                case 0: b = 255; break;
            }
            bmp_set(gbp_bmp->bmpBuffer, x, y, b | b << 8 | b << 16);
        }
    }

    f = fopen(filenameBuff, "wb");
    fwrite(gbp_bmp->bmpBuffer, BMP_SIZE(gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight), 1, f);
    fclose(f);
}

#if 0
void gbp_bmp_rendertest(gbp_bmp_t * gbp_bmp, uint16_t sizex, uint16_t sizey)
{
    FILE *f;

    gbp_bmp->bmpSizeWidth  = sizex;
    gbp_bmp->bmpSizeHeight = sizey;

    bmp_init(gbp_bmp->bmpBuffer, gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight);

    for (uint16_t y = 0; y < gbp_bmp->bmpSizeHeight; y++)
    {
        for (uint16_t x = 0; x < gbp_bmp->bmpSizeWidth; x++)
        {
            float r = y / (float)gbp_bmp->bmpSizeHeight;
            float g = x / (float)gbp_bmp->bmpSizeWidth;
            float b = 1.0f;
            bmp_set(gbp_bmp->bmpBuffer, x, y, bmp_encode(r, g, b));
        }
    }

    f = fopen("test2.bmp", "wb");
    fwrite(gbp_bmp->bmpBuffer, BMP_SIZE(gbp_bmp->bmpSizeWidth, gbp_bmp->bmpSizeHeight), 1, f);
    fclose(f);
}
#endif