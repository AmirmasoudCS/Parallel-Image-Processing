#include <stdio.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "benchmark.h"
#include "filters.h"

int benchmark_grayscale(const Image* input, Image* sequential_output, Image* parallel_output, int repetitions){
    if(repetitions <= 0) return 0;

    const int thread_counts[] = {1, 2, 4, 8};
    const int number_of_configs = 4;
    size_t image_size=(size_t)input->width * input->height * input->channels;
    double sequential_time = 0.0;
    double parallel_times[4];
    double speedups[4];
    double efficiencies[4];
    omp_set_dynamic(0);
    printf("\n========== Grayscale Benchmark ==========\n");
    printf("Image size: %d x %d\n", input->width, input->height);
    printf("Repetitions: %d\n\n", repetitions);

    grayscale_sequential(input, sequential_output);
    double start=omp_get_wtime();
    for(int i = 0 ; i < repetitions ; i++) grayscale_sequential(input, sequential_output);
    double end = omp_get_wtime();
    sequential_time = (end-start)/repetitions;
    printf("Sequential average: %.6f seconds\n\n", sequential_time);
    printf("%-12s %-15s %-15s %-15s\n", "Threads", "Time (seconds)", "Speedup", "Efficiency");
    for(int i = 0 ; i < number_of_configs; i++){
        int threads = thread_counts[i];
        omp_set_num_threads(threads);
        grayscale_parallel(input, parallel_output);
        start=omp_get_wtime();
        for(int j = 0; j < repetitions; j++) grayscale_parallel(input, parallel_output);
        end = omp_get_wtime();
        parallel_times[i] = (end-start)/repetitions;
        speedups[i] = sequential_time / parallel_times[i];
        efficiencies[i] = speedups[i]/threads * 100.0;
        printf("%-12d %-15.6f %-15.2fx %-14.2f%%\n", threads, parallel_times[i], speedups[i], efficiencies[i]);

    }

    int correct = memcmp(sequential_output->pixels, parallel_output->pixels, image_size) == 0;
    printf("\n========== Correctness Check ==========\n");
    if(correct){
        printf("PASS: Sequential and parallel outputs are identical.\n");
    }
    else{
        printf("FAIL: Sequential and parallel outputs differ!\n");
    }

    FILE* file = fopen("results/benchmark_results.jsonl", "a");
    if(file==NULL)printf("Failed to open benchmark results file!\n");
    else{
        time_t timestamp = time(NULL);
        fprintf(file,
            "{"
            "\"timestamp\": %lld,"
            "\"image\": {"
                "\"width\": %d,"
                "\"height\": %d,"
                "\"channels\": %d,"
                "\"total_pixels\": %zu"
            "},"
            "\"repetitions\": %d,"
            "\"sequential_time\": %.9f,"
            "\"parallel_results\": [",
            (long long)timestamp,
            input->width,
            input->height,
            input->channels,
            (size_t)input->width * input->height,
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
                thread_counts[i],
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