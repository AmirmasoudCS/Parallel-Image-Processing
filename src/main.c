#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "filters.h"
#include "benchmark.h"

int main(int argc, char* argv[]){
    if(argc!=3){
        printf("Usage: image_processor <input> <output>\n");
        return 1;
    }

    Image input = {0};
    Image sequential_output = {0};
    Image parallel_output = {0};

    if(!image_load(&input, argv[1])) return 1;

    sequential_output.width = input.width;
    sequential_output.height = input.height;
    sequential_output.channels = input.channels;

    parallel_output.width = input.width;
    parallel_output.height = input.height;
    parallel_output.channels = input.channels;

    size_t image_size = (size_t)input.width * input.height * input.channels;

    sequential_output.pixels = malloc(image_size);
    parallel_output.pixels = malloc(image_size);

    if(sequential_output.pixels==NULL || parallel_output.pixels==NULL){
        printf("Memory allocation failed!\n");
        free(sequential_output.pixels);
        free(parallel_output.pixels);
        image_free(&input);
        return 1;
    }

    int correct = benchmark_grayscale(
        &input,
        &sequential_output,
        &parallel_output,
        10
    );

    if(!image_save_png(&parallel_output, argv[2])){
        correct = 0;
    }

    free(sequential_output.pixels);
    free(parallel_output.pixels);
    image_free(&input);

    if(correct){
        printf("Grayscale conversion completed successfully!\n");
        return 0;
    }

    printf("Grayscale conversion failed!\n");
    return 1;
}