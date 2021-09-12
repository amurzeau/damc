#include "ServerControl.h"

ServerControl::ServerControl() : OscRoot(false), OscConnector(this, true) {}

void ServerControl::init(int baseDelayOutputInstance) {
	struct sockaddr_in dest;
	uv_ip4_addr("127.0.0.1", 2408, &dest);

	uv_tcp_init(uv_default_loop(), &tcpClient);

	tcpClient.data = this;
	tcpConnect.data = this;
	request.data = this;
	writeRequestInProgress = false;
	connected = false;
	this->baseDelayOutputInstance = baseDelayOutputInstance;

	uv_tcp_connect(&tcpConnect, &tcpClient, (const struct sockaddr*) &dest, &ServerControl::onConnect);
}

void ServerControl::stop() {
	uv_close((uv_handle_t*) &tcpClient, nullptr);
}

void ServerControl::onConnect(uv_connect_t* connection, int status) {
	ServerControl* thisInstance = (ServerControl*) connection->data;
	if(status == 0) {
		uv_stream_t* stream = connection->handle;
		printf("Connected to server\n");
		thisInstance->connected = true;
		uv_read_start(stream, &allocCb, &onRead);
		thisInstance->processSendNextMessage();
	} else {
		printf("Connection failed: %d\n", status);
	}
}

void ServerControl::allocCb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
	buf->base = (char*) malloc(size);
	buf->len = size;
}

void ServerControl::onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
	free(buf->base);
}

void ServerControl::onWrite(uv_write_t* req, int status) {
	ServerControl* thisInstance = (ServerControl*) req->data;

	thisInstance->commands.pop_front();
	thisInstance->writeRequestInProgress = false;
	thisInstance->processSendNextMessage();
}

void ServerControl::sendOscData(const uint8_t* data, size_t size) {
	std::string message;
	message.assign(data, data + size);

	commands.push_back(message);

	if(!writeRequestInProgress && connected) {
		processSendNextMessage();
	}
}

void ServerControl::processSendNextMessage() {
	if(commands.empty())
		return;

	std::string& message = commands.front();

	writeRequestInProgress = true;
	uv_buf_t buffer;
	buffer.len = message.size();
	buffer.base = (char*) message.data();
	uv_write(&request, (uv_stream_t*) &tcpClient, &buffer, 1, onWrite);
}

void ServerControl::setRawDelay(int outputInstance, int delay) {
	auto it = configuredDelayPerOutputInstance.find(outputInstance);
	if(it != configuredDelayPerOutputInstance.end()) {
		if(it->second == delay) {
			// Same delay, don't update
			return;
		}
	}

	if(delay < 0) {
		printf("Attempt to set delay %d to %d\n", delay, outputInstance);
		return;
	}

	configuredDelayPerOutputInstance[outputInstance] = delay;

	OscArgument oscArg = delay;
	printf("Setting delay of %d to %d\n", outputInstance, delay);
	sendMessage("/strip/" + std::to_string(outputInstance) + "/filterChain/delay", &oscArg, 1);
}

void ServerControl::updateDelays() {
	int baseDelay = 0;

	auto minValueIt =
	    std::min_element(assignedDelayPerOutputInstance.begin(),
	                     assignedDelayPerOutputInstance.end(),
	                     [](std::pair<int, int> a, std::pair<int, int> b) { return a.second < b.second; });
	if(minValueIt != assignedDelayPerOutputInstance.end() && minValueIt->second < 0) {
		baseDelay = -minValueIt->second;
	}

	setRawDelay(baseDelayOutputInstance, baseDelay);
	for(auto& assignedDelay : assignedDelayPerOutputInstance) {
		setRawDelay(assignedDelay.first, assignedDelay.second + baseDelay);
	}
}

void ServerControl::setClockDrift(int outputInstance, float clockDrift) {
	OscArgument oscArg = clockDrift;
	sendMessage("/strip/" + std::to_string(outputInstance) + "/device/clockDrift", &oscArg, 1);
}

void ServerControl::adjustDelay(int outputInstance, int delay) {
	if(assignedDelayPerOutputInstance.count(outputInstance) == 0)
		assignedDelayPerOutputInstance[outputInstance] = 0;

	assignedDelayPerOutputInstance[outputInstance] += delay;
}
