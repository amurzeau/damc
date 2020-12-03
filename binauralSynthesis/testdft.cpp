#include <stdio.h>
#include <string.h>
#include <vector>

#include "DiscreteFourierTransform.h"

int main(int argc, char* argv[]) {
	std::vector<float> timeDomain;
	std::vector<std::complex<float>> freqDomain;
	std::vector<float> timeDomainResult;

	timeDomain.resize(13516);
	freqDomain.resize(timeDomain.size());
	timeDomainResult.resize(timeDomain.size());

	printf("Start 1\n");
	DiscreteFourierTransform::dft(timeDomain.size(), timeDomain.data(), freqDomain.data());
	printf("Done, do ifft\n");
	DiscreteFourierTransform::idft(freqDomain.size(), freqDomain.data(), timeDomainResult.data());

	if(memcmp(timeDomain.data(), timeDomainResult.data(), timeDomain.size() * sizeof(timeDomain[0])) != 0)
		printf("Wrong DFT\n");
	else
		return 0;

	for(size_t i = 0; i < timeDomain.size(); i++) {
		printf("%.3f %.3f\n", timeDomain[i], timeDomainResult[i]);
	}

	return 0;
}
