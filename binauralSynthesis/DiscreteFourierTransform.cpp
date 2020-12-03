#include "DiscreteFourierTransform.h"
#include <complex>
#include <math.h>

#include "fftw3.h"

void DiscreteFourierTransform::dft(size_t n, const float* timeDomain, std::complex<float>* freqDomain) {
	fftwf_plan plan;
	fftwf_complex* out = reinterpret_cast<fftwf_complex*>(freqDomain);

	static_assert(sizeof(freqDomain[0]) == sizeof(fftwf_complex), "cannot cast std::complex to fftwf_complex");

	plan = fftwf_plan_dft_r2c_1d(n, (float*) timeDomain, out, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
	fftwf_execute(plan);
	fftwf_destroy_plan(plan);
	//	for(size_t k = 0; k < n; k++) {
	//		std::complex<float> sum = 0;
	//		std::complex<float> factor = exp(std::complex<float>(0, -2 * M_PI * k / n));
	//		for(size_t i = 0; i < n; i++) {
	//			sum += timeDomain[i] * pow(factor, std::complex<float>(i, 0));
	//		}
	//		freqDomain[k] = sum;
	//	}
}

void DiscreteFourierTransform::idft(size_t n, const std::complex<float>* freqDomain, float* timeDomain) {
	fftwf_plan plan;
	fftwf_complex* in = (fftwf_complex*) (freqDomain);

	static_assert(sizeof(freqDomain[0]) == sizeof(fftwf_complex), "cannot cast std::complex to fftwf_complex");

	plan = fftwf_plan_dft_c2r_1d(n, in, timeDomain, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
	fftwf_execute(plan);
	fftwf_destroy_plan(plan);

	//	for(size_t k = 0; k < n; k++) {
	//		float sum = 0;
	//		for(size_t i = 0; i < n; i++) {
	//			sum += std::abs(freqDomain[i] * exp(std::complex<float>(0, 2 * M_PI * k * i / n)));
	//		}
	//		timeDomain[k] = sum / n;
	//	}
}
