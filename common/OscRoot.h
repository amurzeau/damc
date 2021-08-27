#pragma once

#include <Osc/OscContainer.h>
#include <list>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>

struct tosc_message_const;

class OscConnector;

class OscRoot : public OscContainer {
public:
	OscRoot(bool notifyAtInit);
	~OscRoot();

	void onOscPacketReceived(const uint8_t* data, size_t size);

	void printAllNodes();
	void triggerAddress(const std::string& address);

	void addConnector(OscConnector* connector);
	void removeConnector(OscConnector* connector);
	void setOnOscValueChanged(std::function<void()> onOscValueChanged);

	void sendMessage(const std::string& address, const OscArgument* argument, size_t number) override;

protected:
	void executeMessage(tosc_message_const* osc);

	bool notifyOscAtInit() override;
	void notifyValueChanged() override;

private:
	std::set<OscConnector*> connectors;
	std::unique_ptr<uint8_t[]> oscOutputMessage;
	size_t oscOutputMaxSize;
	std::function<void()> onOscValueChanged;
	bool doNotifyOscAtInit;
};

class OscConnector {
public:
	OscConnector(OscRoot* oscRoot, bool useSlipProtocol);
	virtual ~OscConnector();

	void sendOscMessage(const uint8_t* data, size_t size);

protected:
	void onOscDataReceived(const uint8_t* data, size_t size);
	virtual void sendOscData(const uint8_t* data, size_t size) = 0;

	OscRoot* getOscRoot() { return oscRoot; }

private:
	OscRoot* oscRoot;
	bool useSlipProtocol;
	bool oscIsEscaping;
	std::vector<uint8_t> oscInputBuffer;
	std::vector<uint8_t> oscOutputBuffer;
};
