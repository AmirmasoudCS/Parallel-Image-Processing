
#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"

int grayscale_sequential(const Image* input, Image* output);

int grayscale_parallel(const Image* input, Image* output);

int threshold_sequential(const Image* input, Image* output, int threshold);

int threshold_parallel(const Image* input, Image* output, int threshold);

#endif
