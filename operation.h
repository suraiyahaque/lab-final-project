#ifndef OPERATION_H
#define OPERATION_H

#include "image.h"

void apply_grayscale(Image *image);
void apply_brightness(Image *image, int value);
void apply_invert(Image *image);
void apply_horizontal_flip(Image *image);
void apply_vertical_flip(Image *image);
Image *apply_rotate_90(const Image *image);
Image *apply_crop(const Image *image, int x, int y, int width, int height);
Image *apply_blur(const Image *image);

#endif