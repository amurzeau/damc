#pragma once

#include <OscRoot.h>
#include <stdint.h>
#include <vector>

class OscRoot;

class OscStatePersist {
public:
	OscStatePersist(OscRoot* oscRoot, std::string fileName);

	void loadState(std::map<std::string, std::set<std::string>>& outputPortConnections);
	void saveState(const std::map<std::string, std::set<std::string>>& outputPortConnections);

private:
	OscRoot* oscRoot;
	std::string saveFileName;
};
