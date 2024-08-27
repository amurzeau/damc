#pragma once

#include <OscRoot.h>
#include <stdint.h>
#include <vector>

class OscRoot;

class OscStatePersist {
public:
	OscStatePersist(OscRoot* oscRoot);

	void loadState(std::string filename);
	void saveState(std::string filename);

private:
	OscRoot* oscRoot;
};
