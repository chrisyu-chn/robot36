
#include <math.h>
#include <stdlib.h>
#include <complex.h> 
#include "window.h"
#include "ddc.h"

void do_ddc(ddc_t *ddc, float *input, float complex *output)
{
	int in = 0;
	ddc->s[ddc->last] = input[in++];
	ddc->last = (ddc->last + 1) < ddc->samples ? ddc->last + 1 : 0;
	ddc->skip += ddc->L;
	// this works only for L <= M
	for (int k = 0; k < ddc->L; k++) {
		float complex sum = 0.0;
		for (int i = ddc->offset, j = ddc->last; i < ddc->taps; i += ddc->L) {
			sum += ddc->b[i] * ddc->s[j];
			j += j ? - 1 : ddc->samples - 1;
		}

		ddc->offset = (ddc->offset + ddc->M) % ddc->L;

		while (ddc->skip < ddc->M) {
			ddc->s[ddc->last] = input[in++];
			ddc->last = (ddc->last + 1) < ddc->samples ? ddc->last + 1 : 0;
			ddc->skip += ddc->L;
		}

		ddc->skip %= ddc->M;
		output[k] = ddc->osc * sum;
		ddc->osc *= ddc->d;
//		ddc->osc /= cabsf(ddc->osc); // not really needed
	}
}
ddc_t *alloc_ddc(float freq, float bw, float step, int taps, int L, int M, float (*window)(float, float))
{
	float lstep = step / (float)L;
	float ostep = step * (float)M / (float)L;
	ddc_t *ddc = malloc(sizeof(ddc_t));
	ddc->taps = taps;
	ddc->samples = (taps + L - 1) / L;
	ddc->b = malloc(sizeof(float complex) * ddc->taps);
	ddc->s = malloc(sizeof(float) * ddc->samples);
	ddc->osc = I;
	ddc->d = cexpf(-I * 2.0 * M_PI * freq * ostep);
	ddc->offset = 0;
	ddc->last = 0;
	ddc->skip = 0;
	ddc->L = L;
	ddc->M = M;
	for (int i = 0; i < ddc->samples; i++)
		ddc->s[i] = 0.0;
	float sum = 0.0;
	for (int i = 0; i < ddc->taps; i++) {
		float N = (float)ddc->taps;
		float n = (float)i;
		float x = n - (N - 1.0) / 2.0;
		float l = 2.0 * M_PI * bw * lstep;
		float w = window(n, ddc->taps);
		float h = 0.0 == x ? l / M_PI : sinf(l * x) / (x * M_PI);
		float b = w * h;
		sum += b;
		complex float o = cexpf(I * 2.0 * M_PI * freq * lstep * n);
		ddc->b[i] = b * o * (float)L;
	}
	for (int i = 0; i < ddc->taps; i++)
		ddc->b[i] /= sum;
	return ddc;
}
void free_ddc(ddc_t *ddc)
{
	free(ddc->b);
	free(ddc->s);
	free(ddc);
}

