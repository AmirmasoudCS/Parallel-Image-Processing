#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "image.h"
#include "filters.h"
#include "benchmark.h"

#define BENCHMARK_RUNS 50
#define DEFAULT_THRESHOLD 128

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

typedef struct{
    int benchmark;
    int threads;   /* 0 = leave to OpenMP default / OMP_NUM_THREADS */
} Options;

static void print_usage(void){
    printf("Usage: image_processor <input> <output> <filter> [threshold] [--benchmark] [--threads N]\n");
    printf("Filters: grayscale, threshold, otsu, all\n");
    printf("  (default)      process the image once and save the result\n");
    printf("  --benchmark    compare sequential vs parallel over %d runs, then save\n", BENCHMARK_RUNS);
    printf("  --threads N    number of OpenMP threads\n");
}

static int parse_int(const char* text, int min, int max, int* out){
    char* end;
    long value = strtol(text, &end, 10);
    if(*text == '\0' || *end != '\0' || value < min || value > max) return 0;
    *out = (int)value;
    return 1;
}

/* Runs one filter either once (normal mode) or as a benchmark.
   Returns 1 on success; result is left in `parallel`. */
static int run_filter(const Options* opt, const char* filter,
                      const Image* input, Image* sequential, Image* parallel,
                      int threshold){
    if(opt->benchmark){
        if(strcmp(filter, "grayscale") == 0)
            return benchmark_grayscale(input, sequential, parallel, BENCHMARK_RUNS);
        if(strcmp(filter, "threshold") == 0)
            return benchmark_threshold(input, sequential, parallel, BENCHMARK_RUNS, threshold);
        if(strcmp(filter, "otsu") == 0)
            return benchmark_otsu(input, sequential, parallel, BENCHMARK_RUNS);
        return 0;
    }

    if(strcmp(filter, "grayscale") == 0)
        return grayscale_parallel(input, parallel);
    if(strcmp(filter, "threshold") == 0)
        return threshold_parallel(input, parallel, threshold);
    if(strcmp(filter, "otsu") == 0){
        int otsu_value = 0;
        int ok = otsu_parallel(input, parallel, &otsu_value);
        if(ok) printf("Otsu threshold: %d\n", otsu_value);
        return ok;
    }
    return 0;
}

int main(int argc, char* argv[]){
    Options opt = {0, 0};
    const char* positional[4];
    int positional_count = 0;

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--benchmark") == 0 || strcmp(argv[i], "-b") == 0){
            opt.benchmark = 1;
        }
        else if(strcmp(argv[i], "--threads") == 0){
            if(i + 1 >= argc || !parse_int(argv[++i], 1, 4096, &opt.threads)){
                printf("--threads needs an integer between 1 and 4096.\n");
                return 1;
            }
        }
        else if(positional_count < 4){
            positional[positional_count++] = argv[i];
        }
        else{
            print_usage();
            return 1;
        }
    }

    if(positional_count < 3){
        print_usage();
        return 1;
    }

    const char* input_path  = positional[0];
    const char* output_path = positional[1];
    const char* filter      = positional[2];
    int has_threshold = (positional_count == 4);

    int is_grayscale = strcmp(filter, "grayscale") == 0;
    int is_threshold = strcmp(filter, "threshold") == 0;
    int is_otsu      = strcmp(filter, "otsu") == 0;
    int is_all       = strcmp(filter, "all") == 0;

    if(!is_grayscale && !is_threshold && !is_otsu && !is_all){
        printf("Unknown filter: %s\n", filter);
        print_usage();
        return 1;
    }

    int threshold = DEFAULT_THRESHOLD;
    if(has_threshold){
        if(!is_threshold){
            printf("Only the threshold filter accepts a threshold value.\n");
            return 1;
        }
        if(!parse_int(positional[3], 0, 255, &threshold)){
            printf("Threshold must be an integer between 0 and 255.\n");
            return 1;
        }
    }

    if(opt.threads > 0) omp_set_num_threads(opt.threads);

    Image input = {0};
    Image sequential_output = {0};
    Image parallel_output = {0};

    if(!image_load(&input, input_path)) return 1;

    size_t image_size = (size_t)input.width * input.height * input.channels;

    parallel_output.width = input.width;
    parallel_output.height = input.height;
    parallel_output.channels = input.channels;
    parallel_output.pixels = malloc(image_size);

    /* the sequential buffer is only needed for benchmarking */
    if(opt.benchmark){
        sequential_output.width = input.width;
        sequential_output.height = input.height;
        sequential_output.channels = input.channels;
        sequential_output.pixels = malloc(image_size);
    }

    if(parallel_output.pixels == NULL || (opt.benchmark && sequential_output.pixels == NULL)){
        printf("Memory allocation failed!\n");
        free(sequential_output.pixels);
        free(parallel_output.pixels);
        image_free(&input);
        return 1;
    }

    int correct = 0;

    if(is_all){
        const char* names[] = {"grayscale", "threshold", "otsu"};
        correct = 1;
        for(int i = 0; i < 3; i++){
            int ok = run_filter(&opt, names[i], &input, &sequential_output,
                                &parallel_output, DEFAULT_THRESHOLD);
            ok = ok && save_all_output(&parallel_output, output_path, names[i]);
            correct = correct && ok;
        }
    }
    else{
        correct = run_filter(&opt, filter, &input, &sequential_output,
                             &parallel_output, threshold);
        if(correct) correct = image_save_png(&parallel_output, output_path);
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