#include <stdio.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "benchmark.h"
#include "filters.h"

typedef int (*FilterFunction)(const Image*, Image*, int, int*);

int grayscale_sequential_wrapper(const Image* input, Image* output, int parameter, int* threshold){
    (void)parameter;
    (void)threshold;
    return grayscale_sequential(input, output);
}

int grayscale_parallel_wrapper(const Image* input, Image* output, int parameter, int* threshold){
    (void)parameter;
    (void)threshold;
    return grayscale_parallel(input, output);
}

int threshold_sequential_wrapper(const Image* input, Image* output, int parameter, int* selected_threshold){
    if(selected_threshold != NULL) *selected_threshold = parameter;
    return threshold_sequential(input, output, parameter);
}

int threshold_parallel_wrapper(const Image* input, Image* output, int parameter, int* selected_threshold){
    if(selected_threshold != NULL) *selected_threshold = parameter;
    return threshold_parallel(input, output, parameter);
}

int otsu_sequential_wrapper(const Image* input, Image* output, int parameter, int* threshold){
    (void)parameter;
    return otsu_sequential(input, output, threshold);
}

int otsu_parallel_wrapper(const Image* input, Image* output, int parameter, int* threshold){
    (void)parameter;
    return otsu_parallel(input, output, threshold);
}

static int benchmark_filter(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions,
    int parameter,
    const char* filter_name,
    FilterFunction sequential_filter,
    FilterFunction parallel_filter
){
    if(repetitions <= 0) return 0;

    const int number_of_configs = 8;
    size_t image_size = (size_t)input->width * input->height * input->channels;

    double sequential_time = 0.0;
    double parallel_times[8];
    double speedups[8];
    double efficiencies[8];

    int sequential_threshold = -1;
    int parallel_threshold = -1;
    int thresholds_identical = 1;

    omp_set_dynamic(0);

    printf("\n========== %s Benchmark ==========\n", filter_name);
    printf("Image size: %d x %d\n", input->width, input->height);
    printf("Repetitions: %d\n\n", repetitions);

    if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;

    double start = omp_get_wtime();

    for(int i = 0; i < repetitions; i++){
        if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;
    }

    double end = omp_get_wtime();
    sequential_time = (end-start)/repetitions;

    printf("Sequential average: %.6f seconds\n", sequential_time);

    if(strcmp(filter_name, "otsu") == 0){
        printf("Selected threshold: %d\n", sequential_threshold);
    }

    printf("\n%-12s %-15s %-15s %-15s\n", "Threads", "Time (seconds)", "Speedup", "Efficiency");

    for(int i = 0; i < number_of_configs; i++){
        int threads = i+1;

        omp_set_num_threads(threads);

        if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;

        start = omp_get_wtime();

        for(int j = 0; j < repetitions; j++){
            if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;
        }

        end = omp_get_wtime();

        parallel_times[i] = (end-start)/repetitions;
        speedups[i] = sequential_time / parallel_times[i];
        efficiencies[i] = speedups[i]/threads * 100.0;

        if(strcmp(filter_name, "otsu") == 0 &&
           parallel_threshold != sequential_threshold){
            thresholds_identical = 0;
        }

        printf("%-12d %-15.6f %-15.2fx %-14.2f%%\n",
            threads,
            parallel_times[i],
            speedups[i],
            efficiencies[i]
        );
    }

    int outputs_identical = memcmp(
        sequential_output->pixels,
        parallel_output->pixels,
        image_size
    ) == 0;

    int correct = outputs_identical && thresholds_identical;

    printf("\n========== Correctness Check ==========\n");

    if(outputs_identical){
        printf("PASS: Sequential and parallel outputs are identical.\n");
    }
    else{
        printf("FAIL: Sequential and parallel outputs differ!\n");
    }

    if(strcmp(filter_name, "otsu") == 0){
        if(thresholds_identical){
            printf("PASS: Sequential and parallel thresholds are identical (%d).\n",
                sequential_threshold);
        }
        else{
            printf("FAIL: Sequential and parallel thresholds differ!\n");
        }
    }

    FILE* file = fopen("results/benchmark_results.jsonl", "a");

    if(file == NULL) printf("Failed to open benchmark results file!\n");
    else{
        time_t timestamp = time(NULL);

        fprintf(file,
            "{"
            "\"timestamp\": %lld,"
            "\"filter\": \"%s\","
            "\"image\": {"
                "\"width\": %d,"
                "\"height\": %d,"
                "\"channels\": %d,"
                "\"total_pixels\": %llu"
            "},"
            "\"threshold\": ",
            (long long)timestamp,
            filter_name,
            input->width,
            input->height,
            input->channels,
            (unsigned long long)input->width * input->height
        );

        if(strcmp(filter_name, "grayscale") == 0){
            fprintf(file, "null");
        }
        else if(strcmp(filter_name, "otsu") == 0){
            fprintf(file, "%d", sequential_threshold);
        }
        else{
            fprintf(file, "%d", parameter);
        }

        fprintf(file,
            ","
            "\"repetitions\": %d,"
            "\"sequential_time\": %.9f,"
            "\"parallel_results\": [",
            repetitions,
            sequential_time
        );

        for(int i = 0; i < number_of_configs; i++){
            fprintf(file,
                "%s{"
                    "\"threads\": %d,"
                    "\"time\": %.9f,"
                    "\"speedup\": %.4f,"
                    "\"efficiency\": %.4f"
                "}",
                i == 0 ? "" : ",",
                i+1,
                parallel_times[i],
                speedups[i],
                efficiencies[i]
            );
        }

        fprintf(file,
            "],"
            "\"outputs_identical\": %s,"
            "\"thresholds_identical\": %s"
            "}\n",
            outputs_identical ? "true" : "false",
            thresholds_identical ? "true" : "false"
        );

        fclose(file);

        printf("\nBenchmark results saved successfully!\n");
    }

    printf("=======================================\n");

    return correct;
}

int benchmark_grayscale(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions
){
    return benchmark_filter(
        input,
        sequential_output,
        parallel_output,
        repetitions,
        0,
        "grayscale",
        grayscale_sequential_wrapper,
        grayscale_parallel_wrapper
    );
}

int benchmark_threshold(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions,
    int threshold
){
    return benchmark_filter(
        input,
        sequential_output,
        parallel_output,
        repetitions,
        threshold,
        "threshold",
        threshold_sequential_wrapper,
        threshold_parallel_wrapper
    );
}

int benchmark_otsu(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions
){
    return benchmark_filter(
        input,
        sequential_output,
        parallel_output,
        repetitions,
        0,
        "otsu",
        otsu_sequential_wrapper,
        otsu_parallel_wrapper
    );
}