#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

Image *image_create(int width, int height)
{
    Image *image;
    size_t pixel_count;
    size_t data_size;

    if (width <= 0 || height <= 0)
        return NULL;

    pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / sizeof(Pixel))
        return NULL;

    data_size = pixel_count * sizeof(Pixel);

    image = malloc(sizeof(Image));

    if (image == NULL)
        return NULL;

    image->width = width;
    image->height = height;

    image->data = malloc(data_size);

    if (image->data == NULL)
    {
        free(image);
        return NULL;
    }

    memset(image->data, 0, data_size);

    return image;
}

void image_free(Image *image)
{
    if (image == NULL)
        return;

    free(image->data);
    free(image);
}

Image *image_copy(const Image *source)
{
    Image *copy;
    size_t pixel_count;

    if (source == NULL || source->data == NULL)
        return NULL;

    copy = image_create(source->width, source->height);

    if (copy == NULL)
        return NULL;

    pixel_count = (size_t)source->width * (size_t)source->height;

    memcpy(copy->data, source->data, pixel_count * sizeof(Pixel));

    return copy;
}

Pixel *image_get_pixel(Image *image, int x, int y)
{
    if (image == NULL || image->data == NULL)
        return NULL;

    if (x < 0 || x >= image->width ||
        y < 0 || y >= image->height)
        return NULL;

    return &image->data[y * image->width + x];
}
const Pixel *image_get_pixel_const(const Image *image,int x,int y){
 if (image == NULL || image->data == NULL)
        return NULL;

    if (x < 0 || x >= image->width ||
        y < 0 || y >= image->height)
        return NULL;

    return &image->data[y * image->width + x];
}