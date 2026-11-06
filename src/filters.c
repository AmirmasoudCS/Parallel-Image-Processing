#include <stdlib.h>
#include <omp.h>
#include "filters.h"

static unsigned char get_grayscale(const Image* input, int i){
    int index = i * input->channels;

    unsigned char r = input->pixels[index];
    unsigned char g = input->pixels[index+1];
    unsigned char b = input->pixels[index+2];

    return (unsigned char)(0.299*r + 0.587*g + 0.114*b);
}

static int find_otsu_threshold(unsigned long long histogram[256], int total_pixels){
    unsigned long long total_sum = 0;

    for(int i = 0; i < 256; i++){
        total_sum += (unsigned long long)i * histogram[i];
    }

    unsigned long long background_sum = 0;
    unsigned long long background_weight = 0;

    double maximum_variance = -1.0;
    int threshold = 0;

    for(int i = 0; i < 256; i++){
        background_weight += histogram[i];

        if(background_weight == 0) continue;

        unsigned long long foreground_weight = total_pixels - background_weight;

        if(foreground_weight == 0) break;

        background_sum += (unsigned long long)i * histogram[i];

        double background_mean = (double)background_sum / background_weight;
        double foreground_mean = (double)(total_sum - background_sum) / foreground_weight;

        double difference = background_mean - foreground_mean;
        double variance = (double)background_weight * foreground_weight * difference * difference;

        if(variance > maximum_variance){
            maximum_variance = variance;
            threshold = i;
        }
    }

    return threshold;
}

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

int threshold_sequential(const Image* input, Image* output, int threshold){
    if(threshold < 0 || threshold > 255) return 0;

    int total_pixels = input->width * input->height;

    for(int i = 0; i < total_pixels; i++){
        int index = i * input->channels;

        unsigned char r = input->pixels[index];
        unsigned char g = input->pixels[index+1];
        unsigned char b = input->pixels[index+2];

        unsigned char gray = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
        unsigned char binary = gray >= threshold ? 255 : 0;

        output->pixels[index] = binary;
        output->pixels[index+1] = binary;
        output->pixels[index+2] = binary;
    }

    return 1;
}

int threshold_parallel(const Image* input, Image* output, int threshold){
    if(threshold < 0 || threshold > 255) return 0;

    int total_pixels = input->width * input->height;

    #pragma omp parallel for schedule(static)
    for(int i = 0; i < total_pixels; i++){
        int index = i * input->channels;

        unsigned char r = input->pixels[index];
        unsigned char g = input->pixels[index+1];
        unsigned char b = input->pixels[index+2];

        unsigned char gray = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
        unsigned char binary = gray >= threshold ? 255 : 0;

        output->pixels[index] = binary;
        output->pixels[index+1] = binary;
        output->pixels[index+2] = binary;
    }

    return 1;
}

int otsu_sequential(const Image* input, Image* output, int* threshold){
    if(input == NULL || output == NULL || threshold == NULL ||
       input->pixels == NULL || output->pixels == NULL) return 0;

    int total_pixels = input->width * input->height;
    unsigned long long histogram[256] = {0};

    for(int i = 0; i < total_pixels; i++){
        unsigned char gray = get_grayscale(input, i);
        histogram[gray]++;
    }

    *threshold = find_otsu_threshold(histogram, total_pixels);

    for(int i = 0; i < total_pixels; i++){
        int index = i * input->channels;
        unsigned char gray = get_grayscale(input, i);
        unsigned char binary = gray > *threshold ? 255 : 0;

        output->pixels[index] = binary;
        output->pixels[index+1] = binary;
        output->pixels[index+2] = binary;
    }

    return 1;
}

int otsu_parallel(const Image* input, Image* output, int* threshold){
    if(input == NULL || output == NULL || threshold == NULL ||
       input->pixels == NULL || output->pixels == NULL) return 0;

    int total_pixels = input->width * input->height;
    int number_of_threads = omp_get_max_threads();

    unsigned long long* local_histograms = calloc(
        (size_t)number_of_threads * 256,
        sizeof(unsigned long long)
    );

    if(local_histograms == NULL) return 0;

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        unsigned long long* local_histogram = local_histograms + thread_id * 256;

        #pragma omp for schedule(static)
        for(int i = 0; i < total_pixels; i++){
            unsigned char gray = get_grayscale(input, i);
            local_histogram[gray]++;
        }
    }

    unsigned long long histogram[256] = {0};

    for(int i = 0; i < number_of_threads; i++){
        for(int j = 0; j < 256; j++){
            histogram[j] += local_histograms[i * 256 + j];
        }
    }

    free(local_histograms);

    *threshold = find_otsu_threshold(histogram, total_pixels);

    #pragma omp parallel for schedule(static)
    for(int i = 0; i < total_pixels; i++){
        int index = i * input->channels;
        unsigned char gray = get_grayscale(input, i);
        unsigned char binary = gray > *threshold ? 255 : 0;

        output->pixels[index] = binary;
        output->pixels[index+1] = binary;
        output->pixels[index+2] = binary;
    }

    return 1;
}