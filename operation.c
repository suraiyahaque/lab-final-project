#include "operation.h"

#include <stdlib.h>

static unsigned char clamp(int value)
{
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return (unsigned char)value;
}

void apply_grayscale(Image *image)
{
    int x;
    int y;
    Pixel *pixel;
    int gray;

    if (image == NULL || image->data == NULL)
        return;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            pixel = image_get_pixel(image, x, y);

            if (pixel == NULL)
                continue;

            gray = (299 * pixel->r +
                    587 * pixel->g +
                    114 * pixel->b) / 1000;

            pixel->r = (unsigned char)gray;
            pixel->g = (unsigned char)gray;
            pixel->b = (unsigned char)gray;
        }
    }
}

void apply_brightness(Image *image, int value)
{
    int x;
    int y;
    Pixel *pixel;

    if (image == NULL || image->data == NULL)
        return;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            pixel = image_get_pixel(image, x, y);

            if (pixel == NULL)
                continue;

            pixel->r = clamp((int)pixel->r + value);
            pixel->g = clamp((int)pixel->g + value);
            pixel->b = clamp((int)pixel->b + value);
        }
    }
}

void apply_invert(Image *image)
{
    int x;
    int y;
    Pixel *pixel;

    if (image == NULL || image->data == NULL)
        return;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            pixel = image_get_pixel(image, x, y);

            if (pixel == NULL)
                continue;

            pixel->r = 255 - pixel->r;
            pixel->g = 255 - pixel->g;
            pixel->b = 255 - pixel->b;
        }
    }
}

void apply_horizontal_flip(Image *image)
{
    int x;
    int y;
    int opposite_x;
    Pixel *left;
    Pixel *right;
    Pixel temp;

    if (image == NULL || image->data == NULL)
        return;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width / 2; x++)
        {
            opposite_x = image->width - 1 - x;

            left = image_get_pixel(image, x, y);
            right = image_get_pixel(image, opposite_x, y);

            if (left == NULL || right == NULL)
                continue;

            temp = *left;
            *left = *right;
            *right = temp;
        }
    }
}

void apply_vertical_flip(Image *image)
{
    int x;
    int y;
    int opposite_y;
    Pixel *top;
    Pixel *bottom;
    Pixel temp;

    if (image == NULL || image->data == NULL)
        return;

    for (y = 0; y < image->height / 2; y++)
    {
        opposite_y = image->height - 1 - y;

        for (x = 0; x < image->width; x++)
        {
            top = image_get_pixel(image, x, y);
            bottom = image_get_pixel(image, x, opposite_y);

            if (top == NULL || bottom == NULL)
                continue;

            temp = *top;
            *top = *bottom;
            *bottom = temp;
        }
    }
}

Image *apply_rotate_90(const Image *image)
{
    Image *rotated;
    int x;
    int y;
    const Pixel *source_pixel;
    Pixel *destination_pixel;

    if (image == NULL || image->data == NULL)
        return NULL;

    rotated = image_create(image->height, image->width);

    if (rotated == NULL)
        return NULL;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            source_pixel = image_get_pixel_const(
                image,
                x,
                y
            );

            destination_pixel = image_get_pixel(
                rotated,
                image->height - 1 - y,
                x
            );

            if (source_pixel != NULL && destination_pixel != NULL)
                *destination_pixel = *source_pixel;
        }
    }

    return rotated;
}

Image *apply_crop(const Image *image, int x, int y, int width, int height)
{
    Image *cropped;
    int crop_x;
    int crop_y;
    const Pixel *source_pixel;
    Pixel *destination_pixel;

    if (image == NULL || image->data == NULL)
        return NULL;

    if (x < 0 || y < 0 ||
        width <= 0 || height <= 0 ||
        x + width > image->width ||
        y + height > image->height)
    {
        return NULL;
    }

    cropped = image_create(width, height);

    if (cropped == NULL)
        return NULL;

    for (crop_y = 0; crop_y < height; crop_y++)
    {
        for (crop_x = 0; crop_x < width; crop_x++)
        {
            source_pixel = image_get_pixel_const(
                image,
                x + crop_x,
                y + crop_y
            );

            destination_pixel = image_get_pixel(
                cropped,
                crop_x,
                crop_y
            );

            if (source_pixel != NULL && destination_pixel != NULL)
                *destination_pixel = *source_pixel;
        }
    }

    return cropped;
}

Image *apply_blur(const Image *image)
{
    Image *blurred;
    int x;
    int y;
    int nx;
    int ny;
    int count;
    int red;
    int green;
    int blue;
    const Pixel *source_pixel;
    Pixel *destination_pixel;

    if (image == NULL || image->data == NULL)
        return NULL;

    blurred = image_create(image->width, image->height);

    if (blurred == NULL)
        return NULL;

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            red = 0;
            green = 0;
            blue = 0;
            count = 0;

            for (ny = y - 1; ny <= y + 1; ny++)
            {
                for (nx = x - 1; nx <= x + 1; nx++)
                {
                    if (nx < 0 || nx >= image->width ||
                        ny < 0 || ny >= image->height)
                    {
                        continue;
                    }

                    source_pixel = image_get_pixel_const(
                        image,
                        nx,
                        ny
                    );

                    if (source_pixel == NULL)
                        continue;

                    red += source_pixel->r;
                    green += source_pixel->g;
                    blue += source_pixel->b;
                    count++;
                }
            }

            destination_pixel = image_get_pixel(
                blurred,
                x,
                y
            );

            if (destination_pixel != NULL && count > 0)
            {
                destination_pixel->r =
                    (unsigned char)(red / count);

                destination_pixel->g =
                    (unsigned char)(green / count);

                destination_pixel->b =
                    (unsigned char)(blue / count);
            }
        }
    }

    return blurred;
}