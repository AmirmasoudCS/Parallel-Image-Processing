
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


static int save_benchmark_result(
    const Image* input,
    int repetitions,
    double sequential_time,
    double parallel_times[],
    double speedups[],
    double efficiencies[],
    int number_of_configs,
    const char* filter_name,
    int parameter,
    int sequential_threshold,
    int outputs_identical,
    int thresholds_identical
){
    const char* filename = "results/benchmark_results.jsonl";
    const char* temporary_filename = "results/benchmark_results.tmp";

    char filter_pattern[64];
    snprintf(filter_pattern, sizeof(filter_pattern), "\"filter\": \"%s\"", filter_name);

    FILE* original = fopen(filename, "r");
    FILE* temporary = fopen(temporary_filename, "w");

    if(temporary == NULL){
        printf("Failed to create temporary benchmark file!\n");
        if(original != NULL) fclose(original);
        return 0;
    }

    if(original != NULL){
        char line[8192];

        while(fgets(line, sizeof(line), original) != NULL){
            if(strstr(line, filter_pattern) == NULL){
                fputs(line, temporary);
            }
        }

        fclose(original);
    }

    time_t timestamp = time(NULL);

    fprintf(temporary,
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
        fprintf(temporary, "null");
    }
    else if(strcmp(filter_name, "otsu") == 0){
        fprintf(temporary, "%d", sequential_threshold);
    }
    else{
        fprintf(temporary, "%d", parameter);
    }

    fprintf(temporary,
        ","
        "\"repetitions\": %d,"
        "\"sequential_time\": %.9f,"
        "\"parallel_results\": [",
        repetitions,
        sequential_time
    );

    for(int i = 0; i < number_of_configs; i++){
        fprintf(temporary,
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

    fprintf(temporary,
        "],"
        "\"outputs_identical\": %s,"
        "\"thresholds_identical\": %s"
        "}\n",
        outputs_identical ? "true" : "false",
        thresholds_identical ? "true" : "false"
    );

    int write_error = ferror(temporary);
    int close_error = fclose(temporary);

    if(write_error || close_error){
        printf("Failed to write benchmark results!\n");
        remove(temporary_filename);
        return 0;
    }

    remove(filename);

    if(rename(temporary_filename, filename) != 0){
        printf("Failed to update benchmark results file!\n");
        return 0;
    }

    printf("\nBenchmark results saved successfully!\n");

    return 1;
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

    double total_sequential_time = 0.0;

    int sequential_threshold = -1;
    int parallel_threshold = -1;
    int thresholds_identical = 1;

    omp_set_dynamic(0);

    printf("\n========== %s Benchmark ==========\n", filter_name);
    printf("Image size: %d x %d\n", input->width, input->height);
    printf("Repetitions: %d\n\n", repetitions);

    for(int i = 0; i < number_of_configs; i++){
        int threads = i+1;

        double config_sequential_time = 0.0;
        double config_parallel_time = 0.0;

        omp_set_num_threads(threads);

        // Warmup runs. Alternate their order between configurations.
        if(i % 2 == 0){
            if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;
            if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;
        }
        else{
            if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;
            if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;
        }

        // Alternate the measurement order on every repetition.
        for(int j = 0; j < repetitions; j++){
            double start;
            double end;

            if(j % 2 == 0){
                start = omp_get_wtime();

                if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;

                end = omp_get_wtime();
                config_sequential_time += end-start;

                start = omp_get_wtime();

                if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;

                end = omp_get_wtime();
                config_parallel_time += end-start;
            }
            else{
                start = omp_get_wtime();

                if(!parallel_filter(input, parallel_output, parameter, &parallel_threshold)) return 0;

                end = omp_get_wtime();
                config_parallel_time += end-start;

                start = omp_get_wtime();

                if(!sequential_filter(input, sequential_output, parameter, &sequential_threshold)) return 0;

                end = omp_get_wtime();
                config_sequential_time += end-start;
            }

            if(strcmp(filter_name, "otsu") == 0 &&
               parallel_threshold != sequential_threshold){
                thresholds_identical = 0;
            }
        }

        parallel_times[i] = config_parallel_time / repetitions;
        total_sequential_time += config_sequential_time;
    }

    // Average sequential time across all configurations.
    sequential_time = total_sequential_time / (number_of_configs * repetitions);

    printf("Sequential average: %.6f seconds\n", sequential_time);

    if(strcmp(filter_name, "otsu") == 0){
        printf("Selected threshold: %d\n", sequential_threshold);
    }

    printf("\n%-12s %-15s %-15s %-15s\n", "Threads", "Time (seconds)", "Speedup", "Efficiency");

    for(int i = 0; i < number_of_configs; i++){
        int threads = i+1;

        speedups[i] = sequential_time / parallel_times[i];
        efficiencies[i] = speedups[i] / threads * 100.0;

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

    int saved = save_benchmark_result(
        input,
        repetitions,
        sequential_time,
        parallel_times,
        speedups,
        efficiencies,
        number_of_configs,
        filter_name,
        parameter,
        sequential_threshold,
        outputs_identical,
        thresholds_identical
    );

    printf("=======================================\n");

    return correct && saved;
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
