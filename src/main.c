
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "filters.h"
#include "benchmark.h"

int main(int argc, char* argv[]){
    if(argc < 4 || argc > 5){
        printf("Usage: image_processor <input> <output> <filter> [threshold]\n");
        printf("Filters: grayscale, threshold\n");
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

    if(sequential_output.pixels == NULL || parallel_output.pixels == NULL){
        printf("Memory allocation failed!\n");
        free(sequential_output.pixels);
        free(parallel_output.pixels);
        image_free(&input);
        return 1;
    }

    int correct = 0;

    if(strcmp(argv[3], "grayscale") == 0){
        if(argc != 4){
            printf("Grayscale does not require a threshold value.\n");
            correct = 0;
        }
        else{
            correct = benchmark_grayscale(
                &input,
                &sequential_output,
                &parallel_output,
                10
            );
        }
    }
    else if(strcmp(argv[3], "threshold") == 0){
        int threshold = 128;

        if(argc == 5){
            char* end;
            long value = strtol(argv[4], &end, 10);

            if(*argv[4] == '\0' || *end != '\0' || value < 0 || value > 255){
                printf("Threshold must be an integer between 0 and 255.\n");
                correct = 0;
            }
            else{
                threshold = (int)value;

                correct = benchmark_threshold(
                    &input,
                    &sequential_output,
                    &parallel_output,
                    10,
                    threshold
                );
            }
        }
        else{
            correct = benchmark_threshold(
                &input,
                &sequential_output,
                &parallel_output,
                10,
                threshold
            );
        }
    }
    else{
        printf("Unknown filter: %s\n", argv[3]);
        printf("Available filters: grayscale, threshold\n");
    }

    if(correct){
        if(!image_save_png(&parallel_output, argv[2])){
            correct = 0;
        }
    }

    free(sequential_output.pixels);
    free(parallel_output.pixels);
    image_free(&input);

    if(correct){
        printf("Image processing completed successfully!\n");
        return 0;
    }

    printf("Image processing failed!\n");
    return 1;
}
