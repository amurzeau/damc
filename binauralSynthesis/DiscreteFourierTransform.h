#ifndef DISCRETEFOURIERTRANSFORM_H

#include <complex>

class DiscreteFourierTransform {
public:
	static void dft(size_t n, const float* timeDomain, std::complex<float>* freqDomain);
	static void idft(size_t n, const std::complex<float>* freqDomain, float* timeDomain);
};

#endif
