#ifndef FILTER_FIR2_H
#define FILTER_FIR2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

typedef float sample2_t;
typedef float sample2_hires_t;

struct filter_fir2_t {
	int coef_bank_size;

	struct coef {
		int16_t size;
		sample2_t coefs[1024];
	} coefs[16];

	unsigned int last_index;
	sample2_hires_t history[1024];
	int upsampling_ratio;
	int downsampling_ratio;
	int modpos;
	int modpos_max;
	int size;
};

static inline void filter_fir2_init(
    struct filter_fir2_t* f, const sample2_t* coefs, int coef_size, int upsampling_ratio, int downsampling_ratio) {
	sample2_t temp_coefs[1024];

	memset(temp_coefs, 0, sizeof(temp_coefs));
	memcpy(temp_coefs, coefs, coef_size * sizeof(coefs[0]));

	memset(f->coefs, 0, sizeof(f->coefs));
	memset(f->history, 0, sizeof(f->history));

	f->last_index = 0;
	f->upsampling_ratio = upsampling_ratio;
	f->downsampling_ratio = downsampling_ratio;
	f->modpos = 0;
	f->coef_bank_size = (coef_size + upsampling_ratio - 1) / upsampling_ratio;
	f->size = f->coef_bank_size * upsampling_ratio;

	int k;
	for(k = 0; k == 0 || 0 != (k * (downsampling_ratio - (upsampling_ratio % downsampling_ratio))) % downsampling_ratio;
	    k++) {
		int bank, j;
		for(bank = 0, j = (k * (downsampling_ratio - (upsampling_ratio % downsampling_ratio))) % downsampling_ratio;
		    j < upsampling_ratio;
		    j += downsampling_ratio, bank++) {
			for(int i = 0; i < f->coef_bank_size; i++) {
				f->coefs[k].coefs[bank * f->coef_bank_size + i] =
				    temp_coefs[i * upsampling_ratio + j % upsampling_ratio];
				/*printf("mod %d bank %d [%d] = [%d]\n",
				       k,
				       bank,
				       bank * f->coef_bank_size + i,
				       i * upsampling_ratio + j % upsampling_ratio);*/
			}
		}
		f->coefs[k].size = bank;
	}
	f->modpos_max = k;
}

static inline void filter_fir2_put(struct filter_fir2_t* f, sample2_hires_t input_left) {
	f->history[--f->last_index & (1024 - 1)] = input_left;
}

static int filter_fir2_get(struct filter_fir2_t* f, sample2_t* outputs) {
	filter_fir2_t::coef* coefs = &f->coefs[f->modpos];

	f->modpos++;
	if(f->modpos >= f->modpos_max)
		f->modpos -= f->modpos_max;

	sample2_t* coefs_vals = coefs->coefs;

	for(int i = 0; i < coefs->size; i++) {
		sample2_hires_t sum = 0;

		for(int j = 0; j < f->coef_bank_size; j++) {
			sum += f->history[(f->last_index + j) & (1024 - 1)] * *coefs_vals;
			coefs_vals++;
		}
		outputs[i] = sum;
	};

	return coefs->size;
}

#endif
