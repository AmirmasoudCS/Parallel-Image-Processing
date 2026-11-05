#ifndef IMAGE_H
#define IMAGE_H

typedef struct{
    int width;
    int height;
    int channels;
    unsigned char* pixels;
} Image;

int image_load(Image* image, const char* filename);

int image_save_png(const Image* image, const char* filename);

void image_free(Image* image);

#endif