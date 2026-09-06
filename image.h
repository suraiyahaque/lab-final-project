#ifndef IMAGE_H
#define IMAGE_H

typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

typedef struct
{
    int width;
    int height;
    Pixel *data;
} Image;

Image *image_create(int width, int height);
void image_free(Image *image);
Image *image_copy(const Image *source);
Pixel *image_get_pixel(Image *image, int x, int y);
const Pixel *image_get_pixel_const(const Image *image, int x, int y);

#endif