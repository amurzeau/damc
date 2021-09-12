#ifndef SERVERCONTROL_H

#include "uv.h"
#include <OscRoot.h>
#include <list>
#include <string>
#include <unordered_map>

class ServerControl : public OscRoot, public OscConnector {
public:
	ServerControl();

	void init(int baseDelayOutputInstance);
	void stop();

	void setClockDrift(int outputInstance, float clockDrift);
	void adjustDelay(int outputInstance, int delay);
	void updateDelays();

protected:
	static void onConnect(uv_connect_t* connection, int status);
	static void allocCb(uv_handle_t* handle, size_t size, uv_buf_t* buf);
	static void onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
	static void onWrite(uv_write_t* req, int status);

	void sendOscData(const uint8_t* data, size_t size) override;
	void processSendNextMessage();

	void setRawDelay(int outputInstance, int delay);

private:
	uv_tcp_t tcpClient;
	uv_connect_t tcpConnect;
	uv_write_t request;
	bool connected;
	bool writeRequestInProgress;
	std::list<std::string> commands;

	int baseDelayOutputInstance;
	std::unordered_map<int, int> assignedDelayPerOutputInstance;
	std::unordered_map<int, int> configuredDelayPerOutputInstance;
};

#endif
