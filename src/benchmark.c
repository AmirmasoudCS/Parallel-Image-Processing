
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "benchmark.h"
#include "filters.h"

typedef int (*FilterFunction)(const Image*, Image*, int);

int grayscale_sequential_wrapper(const Image* input, Image* output, int parameter){
    (void)parameter;
    return grayscale_sequential(input, output);
}

int grayscale_parallel_wrapper(const Image* input, Image* output, int parameter){
    (void)parameter;
    return grayscale_parallel(input, output);
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

    omp_set_dynamic(0);

    printf("\n========== %s Benchmark ==========\n", filter_name);
    printf("Image size: %d x %d\n", input->width, input->height);
    printf("Repetitions: %d\n\n", repetitions);

    if(!sequential_filter(input, sequential_output, parameter)) return 0;

    double start = omp_get_wtime();

    for(int i = 0; i < repetitions; i++){
        if(!sequential_filter(input, sequential_output, parameter)) return 0;
    }

    double end = omp_get_wtime();
    sequential_time = (end-start)/repetitions;

    printf("Sequential average: %.6f seconds\n\n", sequential_time);
    printf("%-12s %-15s %-15s %-15s\n", "Threads", "Time (seconds)", "Speedup", "Efficiency");

    for(int i = 0; i < number_of_configs; i++){
        int threads = i+1;

        omp_set_num_threads(threads);

        if(!parallel_filter(input, parallel_output, parameter)) return 0;

        start = omp_get_wtime();

        for(int j = 0; j < repetitions; j++){
            if(!parallel_filter(input, parallel_output, parameter)) return 0;
        }

        end = omp_get_wtime();

        parallel_times[i] = (end-start)/repetitions;
        speedups[i] = sequential_time / parallel_times[i];
        efficiencies[i] = speedups[i]/threads * 100.0;

        printf("%-12d %-15.6f %-15.2fx %-14.2f%%\n",
            threads,
            parallel_times[i],
            speedups[i],
            efficiencies[i]
        );
    }

    int correct = memcmp(
        sequential_output->pixels,
        parallel_output->pixels,
        image_size
    ) == 0;

    printf("\n========== Correctness Check ==========\n");

    if(correct){
        printf("PASS: Sequential and parallel outputs are identical.\n");
    }
    else{
        printf("FAIL: Sequential and parallel outputs differ!\n");
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
            "\"repetitions\": %d,"
            "\"sequential_time\": %.9f,"
            "\"parallel_results\": [",
            (long long)timestamp,
            filter_name,
            input->width,
            input->height,
            input->channels,
            (unsigned long long)input->width * input->height,
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
            "\"outputs_identical\": %s"
            "}\n",
            correct ? "true" : "false"
        );

        fclose(file);

        printf("Benchmark results saved successfully!\n");
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
        threshold_sequential,
        threshold_parallel
    );
}
