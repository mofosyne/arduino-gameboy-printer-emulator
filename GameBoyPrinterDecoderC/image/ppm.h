/* 24-bit PPM (Bitmap) ANSI C header library
 * This is free and unencumbered software released into the public domain.
 */
// Work in progress

static void
ppm_write(const unsigned char *buf, int sizex, int sizey, FILE *f)
{
    fprintf(f, "P6\n%d %d\n255\n", sizex, sizey);
    fwrite(buf, sizex*3, sizey, f);
    fflush(f);
}