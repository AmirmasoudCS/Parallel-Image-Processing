#include <stdio.h>
#include "image.h"

int main(int argc, char* argv[]){
    if(argc!=3){
        printf("Usage: image_processor <input> <output>\n");
        return 1;
    }
    Image image = {0};
    if(!image_load(&image, argv[1])) return 1;
    if(!image_save_png(&image, argv[2])){
        image_free(&image);
        return 1;
    }
    image_free(&image);
    printf("Image processing completed!\n");
    return 0;
}