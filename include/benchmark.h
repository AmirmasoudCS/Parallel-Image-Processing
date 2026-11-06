#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "image.h"

int benchmark_grayscale(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions
);

int benchmark_threshold(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions,
    int threshold
);

int benchmark_otsu(
    const Image* input,
    Image* sequential_output,
    Image* parallel_output,
    int repetitions
);

#endif