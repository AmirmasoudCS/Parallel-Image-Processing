#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "filters.h"

int main(int argc, char* argv[]){
    if(argc!=3){
        printf("Usage: image_processor <input> <output>\n");
        return 1;
    }
    Image input = {0};
    Image output = {0};
    if(!image_load(&input, argv[1])) return 1;

    output.width = input.width;
    output.height = input.height;
    output.channels = input.channels;

    size_t image_size = (size_t)output.width * output.height * output.channels;
    output.pixels = malloc(image_size);
    if(output.pixels==NULL){
        printf("Memory allocation failed!\n");
        image_free(&input);
        return 1;
    }

    grayscale_sequential(&input, &output);

    if(!image_save_png(&output, argv[2])){
        free(output.pixels);
        image_free(&input);
        return 1;
    }

    free(output.pixels);
    image_free(&input);
    printf("Grayscale conversion completed!\n");
    
    return 0;
}