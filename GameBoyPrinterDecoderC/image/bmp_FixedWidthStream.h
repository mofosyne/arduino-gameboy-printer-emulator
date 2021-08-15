/* 24-bit BMP (Bitmap) ANSI C header library
 * This is free and unencumbered software released into the public domain.
 * https://github.com/skeeto/bmp
 */
// MODIFICATION: Allow for delayed writing of the header section while
//               an image of unknown height but known width is streamed in
// BMP_COMPAT removed as it is pointless when only sticking to streaming
// This was modified to make it easier to stream to file in embedded system
// chunk by chunk... assumes fixed width but variable height

#ifndef BMP_H
#define BMP_H

#define BMP_SIZE(w, h) ((h) * ((w) * 3 + (((w) * -3UL) & 3)) + 14 + 40)

#define BMP_PIXEL_START_OFFSET 54
#define BMP_PIXEL_BUFF_SIZE(bufw, bufh) ((bufh) * ((bufw) * 3 + (((bufw) * -3UL) & 3)))

static unsigned long
bmp_size(long width, long height)
{
    long pad = ((width % 4) * -3UL) & 3; /* Overflow-safe */
    if (width < 1 || height < 1)
        return 0; /* Illegal size */
    else if (width > ((0x7fffffffL - 14 - 40) / height - pad) / 3)
        return 0; /* Overflow */
    else
        return height * (width * 3 + pad) + 14 + 40;
}

static void
bmp_header(unsigned char buf[], long width, long height)
{
    long pad;
    unsigned long size;
    unsigned long uw = width;
    unsigned long uh = -height;

    /* bfType */
    buf[0] = 0x42u; ///< 'B'
    buf[1] = 0x4Du; ///< 'M'

    /* bfSize */
    pad  = (width * -3UL) & 3;
    size = height * (width * 3 + pad) + 14 + 40;
    buf[2] = (unsigned char)(size >>  0);
    buf[3] = (unsigned char)(size >>  8);
    buf[4] = (unsigned char)(size >> 16);
    buf[5] = (unsigned char)(size >> 24);

    /* bfReserved1 + bfReserved2 */
    buf[6] = 0x00u;
    buf[7] = 0x00u;
    buf[8] = 0x00u;
    buf[9] = 0x00u;

    /* bfOffBits */
    buf[10] = 0x36u;
    buf[11] = 0x00u;
    buf[12] = 0x00u;
    buf[13] = 0x00u;

    /* biSize */
    buf[14] = 0x28u;
    buf[15] = 0x00u;
    buf[16] = 0x00u;
    buf[17] = 0x00u;

    /* biWidth */
    buf[18] = uw >>  0;
    buf[19] = uw >>  8;
    buf[20] = uw >> 16;
    buf[21] = uw >> 24;

    /* biHeight */
    buf[22] = uh >>  0;
    buf[23] = uh >>  8;
    buf[24] = uh >> 16;
    buf[25] = uh >> 24;

    /* biPlanes */
    buf[26] = 0x01;
    buf[27] = 0x00;

    /* biBitCount */
    buf[28] = 0x18;
    buf[29] = 0x00;

    /* biCompression */
    buf[30] = 0x00;
    buf[31] = 0x00;
    buf[32] = 0x00;
    buf[33] = 0x00;

    /* biSizeImage */
    buf[34] = 0x00;
    buf[35] = 0x00;
    buf[36] = 0x00;
    buf[37] = 0x00;

    /* biXPelsPerMeter */
    buf[38] = 0x00;
    buf[39] = 0x00;
    buf[40] = 0x00;
    buf[41] = 0x00;

    /* biYPelsPerMeter */
    buf[42] = 0x00;
    buf[43] = 0x00;
    buf[44] = 0x00;
    buf[45] = 0x00;

    /* biClrUsed */
    buf[46] = 0x00;
    buf[47] = 0x00;
    buf[48] = 0x00;
    buf[49] = 0x00;

    /* biClrImportant */
    buf[50] = 0x00;
    buf[51] = 0x00;
    buf[52] = 0x00;
    buf[53] = 0x00;
}

static long
bmp_pixelBufferSize(unsigned long width, long height)
{
    const long pad = (width * -3UL) & 3;
    return height * (width * 3 + pad);
}

static void
bmp_set(unsigned char buf[], unsigned long width, long x, long y, unsigned long color)
{
    long pad = (width * -3UL) & 3;
    long byteOffset = (y) * (width * 3 + pad) + x * 3;
    buf[byteOffset + 0]  = color >>  0;
    buf[byteOffset + 1]  = color >>  8;
    buf[byteOffset + 2]  = color >> 16;
}

static unsigned long
bmp_get(unsigned char buf[], unsigned long width, long x, long y)
{
    long pad = (width * -3UL) & 3;
    long byteOffset = (y) * (width * 3 + pad) + x * 3;
    return (unsigned long)buf[byteOffset + 0] <<  0 |
           (unsigned long)buf[byteOffset + 1] <<  8 |
           (unsigned long)buf[byteOffset + 2] << 16;
}

static unsigned long
bmp_encode(float r, float g, float b)
{
    unsigned long ur = r * 255.0f + 0.5f;
    unsigned long ug = g * 255.0f + 0.5f;
    unsigned long ub = b * 255.0f + 0.5f;
    return ub | ug << 8 | ur << 16;
}

static void
bmp_decode(unsigned long c, float *r, float *g, float *b)
{
    unsigned long ur = c >> 16 & 0xff;
    unsigned long ug = c >>  8 & 0xff;
    unsigned long ub = c >>  0 & 0xff;
    *r = ur / 255.0f;
    *g = ug / 255.0f;
    *b = ub / 255.0f;
}

#endif /* BMP_H */
