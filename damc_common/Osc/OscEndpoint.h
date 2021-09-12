#pragma once

#include "OscNode.h"

class OscEndpoint : public OscNode {
public:
	using OscNode::OscNode;
	void setCallback(std::function<void(const std::vector<OscArgument>&)> onExecute);

	void execute(const std::vector<OscArgument>& arguments) override;

	std::string getAsString() const override { return std::string{}; }

private:
	std::function<void(const std::vector<OscArgument>&)> onExecute;
};
