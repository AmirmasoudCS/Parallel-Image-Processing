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
