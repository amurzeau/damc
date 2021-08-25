#pragma once

#include <OscRoot.h>
#include <stdint.h>
#include <vector>

class OscRoot;

class OscStatePersist {
public:
	OscStatePersist(OscRoot* oscRoot, std::string fileName);

	void loadState();
	void saveState();

private:
	OscRoot* oscRoot;
	std::string saveFileName;
};
