#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"

int grayscale_sequential(const Image* input, Image* output);

int grayscale_parallel(const Image* input, Image* output);

#endif