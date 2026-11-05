#include <stdio.h>
#include <string.h>
#include <omp.h>
#include "benchmark.h"
#include "filters.h"

int benchmark_grayscale(const Image* input, Image* sequential_output, Image* parallel_output, int repetitions){
    const int thread_counts[] = {1, 2, 4, 8};
    const int number_of_configs = 4;
    size_t image_size=(size_t)input->width * input->height * input->channels;
    double start, end;
    double sequential_time = 0.0;
    omp_set_dynamic(0);
    printf("\n========== Grayscale Benchmark ==========\n");
    printf("Image size: %d x %d\n", input->width, input->height);
    printf("Repetitions: %d\n\n", repetitions);

    grayscale_sequential(input, sequential_output);
    start=omp_get_wtime();
    for(int i = 0 ; i < repetitions ; i++) grayscale_sequential(input, sequential_output);
    end = omp_get_wtime();
    sequential_time = (end-start)/repetitions;
    printf("Sequential average: %.6f seconds\n\n", sequential_time);
    printf("%-12s %-15s %-15s %-15s\n", "Threads", "Time (seconds)", "Speedup", "Efficiency");
    for(int i = 0 ; i < number_of_configs; i++){
        int threads = thread_counts[i];
        omp_set_num_threads(threads);
        grayscale_parallel(input, parallel_output);
        start=omp_get_wtime();
        for(int j = 0; j < repetitions; j++){
            grayscale_parallel(input, parallel_output);
            end = omp_get_wtime();
            double parallel_time = (end-start)/repetitions;
            double speedup = sequential_time / parallel_time;
            double efficiency = speedup/threads * 100.0;
            printf("%-12d %-15.6f %-15.2fx %-14.2f%%\n", threads, parallel_time, speedup, efficiency);
        }
    }

    int correct = memcmp(sequential_output->pixels, parallel_output->pixels, image_size) == 0;
    printf("\n========== Correctness Check ==========\n");
    if(correct){
        printf("PASS: Sequential and parallel outputs are identical.\n");
    }
    else{
        printf("FAIL: Sequential and parallel outputs differ!\n");
    }
    printf("=======================================\n");

    return correct;
}