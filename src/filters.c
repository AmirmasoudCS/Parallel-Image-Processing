#include "filters.h"

int grayscale_sequential(const Image* input, Image* output){
    int total_pixels = input->width * input->height;
    for(int i = 0; i < total_pixels; i++){
        int index = i * input->channels;
        unsigned char r = input->pixels[index];
        unsigned char g = input->pixels[index+1];
        unsigned char b = input->pixels[index+2];
        unsigned char gray = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
        output->pixels[index] = gray;
        output->pixels[index+1] = gray;
        output->pixels[index+2] = gray;
    }
    return 1;
}

int grayscale_parallel(const Image* input, Image* output){
    int total_pixels = input->width * input->height;
    #pragma omp parallel for schedule(static)
    for(int i=0; i<total_pixels; i++){
        int index = i * input->channels;
        unsigned char r = input->pixels[index];
        unsigned char g = input->pixels[index+1];
        unsigned char b = input->pixels[index+2];
        unsigned char gray = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
        output->pixels[index] = gray;
        output->pixels[index+1] = gray;
        output->pixels[index+2] = gray;
    }
    return 1;
}