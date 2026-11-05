#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "image.h"

int benchmark_grayscale(const Image* input, Image* sequential_output, Image* parallel_output, int repetitions);

#endif 