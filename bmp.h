#ifndef BMP_H
#define BMP_H

#include "image.h"

Image *bmp_load(const char *filename);
int bmp_save(const char *filename, const Image *image);

#endif