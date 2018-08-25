#ifndef FILTER_H
#define FILTER_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

struct filter_farrow_t {
	float delay;
	float history[4];
	unsigned int currentIndex;
};

static void filter_farrow_init(struct filter_farrow_t* filter) {
	memset(filter, 0, sizeof(*filter));
}

static int filter_farrow_get_out_size(size_t count, float ratio, float x0) {
	return ceil((count + x0) * ratio);
}

static size_t filter_farrow_resample_put(
    struct filter_farrow_t* filter, int16_t input, float ratio, float x0, int16_t* output, int stride) {
	filter->history[filter->currentIndex] = input / 32768.0;
	filter->currentIndex = (filter->currentIndex + 1) & 0x3;

	float s[4] = {filter->history[(filter->currentIndex) & 0x3],
	              filter->history[(filter->currentIndex - 3) & 0x3],
	              filter->history[(filter->currentIndex - 2) & 0x3],
	              filter->history[(filter->currentIndex - 1) & 0x3]};

	x0 += filter->delay;

	int outCount = filter_farrow_get_out_size(1, ratio, x0);
	float ratioinv = 1 / ratio;

	short i;
	float k;
	for(i = 0, k = -x0; k >= 0 && k < 1.0; k += ratioinv, i += stride) {
		const int n = 3;
		float d = 1 - k;
		float a0 = s[n - 1];
		float a3 = (s[n] - s[n - 3]) / 6.0 + 0.5 * (s[n - 2] - s[n - 1]);
		float a1 = 0.5 * (s[n] - s[n - 2]) - a3;
		float a2 = s[n] - s[n - 1] - a3 - a1;
		output[i] = (d * (d * (-d * a3 + a2) - a1) + a0) * 32768.0;
	}

	filter->delay = x0 + 1 - float(outCount) * ratioinv;

	return (int) outCount;
}
#endif
