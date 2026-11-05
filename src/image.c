#include <stdio.h>
#include "image.h"
#include "stb_image.h"
#include "stb_image_write.h"

int image_load(Image* image, const char* filename){
    int original_channels;
    image->pixels= stbi_load(filename, &image->width, &image->height, &original_channels, 3);
    if(image->pixels==NULL){
        printf("Fialed to load image: %s\n", filename);
        printf("Reason: %s", stbi_failure_reason());
        return 0;
    }
    image->channels = 3;
    printf("Image loaded successfully!\nWidth: %d\nHeight: %d\nOriginal Channels: %d\nProcessing Channels: %d\n", image->width, image->height, original_channels, image->channels);
    return 1;
}

int image_save_png(const Image* image, const char* filename){
    int result = stbi_write_png(filename, image->width, image->height, image->channels, image->pixels, image->width * image->channels);
    if(!result){
        printf("Failed to save the image: %s\n", filename);
        return 0;
    }
    printf("Image saved: %s\n", filename);
    return 1;
}

void free_image(Image* image){
    if(image->pixels!= NULL){
        stbi_image_free(image->pixels);
        image->pixels=NULL;
    }
    image->width=0;
    image->height=0;
    image->channels=0;
}