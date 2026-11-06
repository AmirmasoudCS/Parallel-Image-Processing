
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "filters.h"
#include "benchmark.h"


static int save_all_output(
    const Image* output,
    const char* filename,
    const char* filter_name
){
    char output_filename[1024];

    const char* extension = strrchr(filename, '.');
    const char* separator = strrchr(filename, '/');
    const char* windows_separator = strrchr(filename, '\\');

    if(windows_separator != NULL &&
       (separator == NULL || windows_separator > separator)){
        separator = windows_separator;
    }

    if(extension != NULL && separator != NULL && extension < separator){
        extension = NULL;
    }

    int result;

    if(extension != NULL){
        int base_length = (int)(extension - filename);

        result = snprintf(
            output_filename,
            sizeof(output_filename),
            "%.*s_%s%s",
            base_length,
            filename,
            filter_name,
            extension
        );
    }
    else{
        result = snprintf(
            output_filename,
            sizeof(output_filename),
            "%s_%s",
            filename,
            filter_name
        );
    }

    if(result < 0 || result >= (int)sizeof(output_filename)){
        printf("Output filename is too long!\n");
        return 0;
    }

    if(!image_save_png(output, output_filename)){
        printf("Failed to save %s output!\n", filter_name);
        return 0;
    }

    printf("Output saved to: %s\n", output_filename);

    return 1;
}


int main(int argc, char* argv[]){
    if(argc < 4 || argc > 5){
        printf("Usage: image_processor <input> <output> <filter> [threshold]\n");
        printf("Filters: grayscale, threshold, otsu, all\n");
        return 1;
    }

    if(strcmp(argv[3], "all") == 0 && argc != 4){
        printf("The all option does not accept a threshold value.\n");
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
        }
        else{
            correct = benchmark_grayscale(
                &input,
                &sequential_output,
                &parallel_output,
                50
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
            }
            else{
                threshold = (int)value;

                correct = benchmark_threshold(
                    &input,
                    &sequential_output,
                    &parallel_output,
                    50,
                    threshold
                );
            }
        }
        else{
            correct = benchmark_threshold(
                &input,
                &sequential_output,
                &parallel_output,
                50,
                threshold
            );
        }
    }
    else if(strcmp(argv[3], "otsu") == 0){
        if(argc != 4){
            printf("Otsu thresholding does not require a threshold value.\n");
        }
        else{
            correct = benchmark_otsu(
                &input,
                &sequential_output,
                &parallel_output,
                50
            );
        }
    }
    else if(strcmp(argv[3], "all") == 0){
        int grayscale_correct = benchmark_grayscale(
            &input,
            &sequential_output,
            &parallel_output,
            50
        );

        int grayscale_saved = 0;

        if(grayscale_correct){
            grayscale_saved = save_all_output(
                &parallel_output,
                argv[2],
                "grayscale"
            );
        }

        int threshold_correct = benchmark_threshold(
            &input,
            &sequential_output,
            &parallel_output,
            50,
            128
        );

        int threshold_saved = 0;

        if(threshold_correct){
            threshold_saved = save_all_output(
                &parallel_output,
                argv[2],
                "threshold"
            );
        }

        int otsu_correct = benchmark_otsu(
            &input,
            &sequential_output,
            &parallel_output,
            50
        );

        int otsu_saved = 0;

        if(otsu_correct){
            otsu_saved = save_all_output(
                &parallel_output,
                argv[2],
                "otsu"
            );
        }

        correct = grayscale_correct && grayscale_saved &&
                  threshold_correct && threshold_saved &&
                  otsu_correct && otsu_saved;
    }
    else{
        printf("Unknown filter: %s\n", argv[3]);
        printf("Available filters: grayscale, threshold, otsu, all\n");
    }

    if(correct && strcmp(argv[3], "all") != 0){
        if(!image_save_png(&parallel_output, argv[2])){
            correct = 0;
        }
    }

    free(sequential_output.pixels);
    free(parallel_output.pixels);
    image_free(&input);

    if(correct){
        printf("\nImage processing completed successfully!\n");
        return 0;
    }

    printf("\nImage processing failed!\n");
    return 1;
}
