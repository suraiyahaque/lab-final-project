#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#pragma pack(push, 1)

typedef struct
{
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPFileHeader;

typedef struct
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t important_colors;
} BMPInfoHeader;

#pragma pack(pop)

static int read_headers(FILE *file, BMPFileHeader *file_header,
                        BMPInfoHeader *info_header)
{
    if (fread(file_header, sizeof(BMPFileHeader), 1, file) != 1)
        return 0;

    if (fread(info_header, sizeof(BMPInfoHeader), 1, file) != 1)
        return 0;

    return 1;
}

Image *bmp_load(const char *filename)
{
    FILE *file;
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    Image *image;
    int width;
    int height;
    int top_down;
    int padding;
    int y;
    int x;

    file = fopen(filename, "rb");

    if (file == NULL)
        return NULL;

    if (!read_headers(file, &file_header, &info_header))
    {
        fclose(file);
        return NULL;
    }

    if (file_header.type != 0x4D42)
    {
        fclose(file);
        return NULL;
    }

    if (info_header.size != 40)
    {
        fclose(file);
        return NULL;
    }

    if (info_header.planes != 1 ||
        info_header.bits_per_pixel != 24 ||
        info_header.compression != 0)
    {
        fclose(file);
        return NULL;
    }

    if (info_header.width <= 0 || info_header.height == 0)
    {
        fclose(file);
        return NULL;
    }

    width = info_header.width;

    if (info_header.height < 0)
    {
        height = -info_header.height;
        top_down = 1;
    }
    else
    {
        height = info_header.height;
        top_down = 0;
    }

    image = image_create(width, height);

    if (image == NULL)
    {
        fclose(file);
        return NULL;
    }

    if (fseek(file, (long)file_header.offset, SEEK_SET) != 0)
    {
        image_free(image);
        fclose(file);
        return NULL;
    }

    padding = (4 - (width * 3) % 4) % 4;

    for (y = 0; y < height; y++)
    {
        int image_y;

        if (top_down)
            image_y = y;
        else
            image_y = height - 1 - y;

        for (x = 0; x < width; x++)
        {
            unsigned char b;
            unsigned char g;
            unsigned char r;
            Pixel *pixel;

            if (fread(&b, sizeof(unsigned char), 1, file) != 1 ||
                fread(&g, sizeof(unsigned char), 1, file) != 1 ||
                fread(&r, sizeof(unsigned char), 1, file) != 1)
            {
                image_free(image);
                fclose(file);
                return NULL;
            }

            pixel = image_get_pixel(image, x, image_y);

            if (pixel == NULL)
            {
                image_free(image);
                fclose(file);
                return NULL;
            }

            pixel->r = r;
            pixel->g = g;
            pixel->b = b;
        }

        if (fseek(file, padding, SEEK_CUR) != 0)
        {
            image_free(image);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);

    return image;
}

int bmp_save(const char *filename, const Image *image)
{
    FILE *file;
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    int padding;
    int row_size;
    int image_size;
    int y;
    int x;

    if (filename == NULL || image == NULL ||
        image->data == NULL ||
        image->width <= 0 || image->height <= 0)
    {
        return -1;
    }

    file = fopen(filename, "wb");

    if (file == NULL)
        return -1;

    padding = (4 - (image->width * 3) % 4) % 4;
    row_size = image->width * 3 + padding;
    image_size = row_size * image->height;

    file_header.type = 0x4D42;
    file_header.size = sizeof(BMPFileHeader) +
                       sizeof(BMPInfoHeader) +
                       image_size;
    file_header.reserved1 = 0;
    file_header.reserved2 = 0;
    file_header.offset = sizeof(BMPFileHeader) +
                         sizeof(BMPInfoHeader);

    info_header.size = sizeof(BMPInfoHeader);
    info_header.width = image->width;
    info_header.height = image->height;
    info_header.planes = 1;
    info_header.bits_per_pixel = 24;
    info_header.compression = 0;
    info_header.image_size = image_size;
    info_header.x_pixels_per_meter = 2835;
    info_header.y_pixels_per_meter = 2835;
    info_header.colors_used = 0;
    info_header.important_colors = 0;

    if (fwrite(&file_header, sizeof(BMPFileHeader), 1, file) != 1)
    {
        fclose(file);
        return -1;
    }

    if (fwrite(&info_header, sizeof(BMPInfoHeader), 1, file) != 1)
    {
        fclose(file);
        return -1;
    }

    for (y = image->height - 1; y >= 0; y--)
    {
        for (x = 0; x < image->width; x++)
        {
            const Pixel *pixel;

            pixel = image_get_pixel_const(image, x, y);

            if (pixel == NULL)
            {
                fclose(file);
                return -1;
            }

            if (fwrite(&pixel->b, sizeof(unsigned char), 1, file) != 1 ||
                fwrite(&pixel->g, sizeof(unsigned char), 1, file) != 1 ||
                fwrite(&pixel->r, sizeof(unsigned char), 1, file) != 1)
            {
                fclose(file);
                return -1;
            }
        }

        for (int i = 0; i < padding; i++)
        {
            unsigned char zero = 0;

            if (fwrite(&zero, sizeof(unsigned char), 1, file) != 1)
            {
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file);

    return 0;
}