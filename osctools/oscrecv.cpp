
#include "tinyosc.h"
#include <uv.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
typedef SOCKET socket_t;
typedef int socketaddr_len_t;
#else
#include <unistd.h>
typedef int socket_t;
typedef socklen_t socketaddr_len_t;
#endif

void close_socket(int s) {
#ifndef _WIN32
	close(s);
#else
	::closesocket(s);
#endif
}

int main() {
	socket_t sock_fd;
	char recvBuffer[65536];

#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	struct sockaddr_in sin;

	// Setup socket server
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_fd == (socket_t) -1)
		return 1;

	memset(&sin, 0, sizeof(sin));
	uv_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);
	sin.sin_port = htons(10000);
	sin.sin_family = AF_INET;

	if(bind(sock_fd, (struct sockaddr*) &sin, sizeof(sin)) == -1)
		return 2;

	struct sockaddr_in recvaddr;
	socketaddr_len_t recvaddr_size = sizeof(recvaddr);
	int recvsize;
	while((recvsize = recvfrom(
	           sock_fd, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*) &recvaddr, &recvaddr_size)) >= 0) {
		if(tosc_isBundle(recvBuffer)) {
			tosc_bundle_const bundle;
			tosc_parseBundle(&bundle, recvBuffer, recvsize);

			tosc_message_const osc;
			while(tosc_getNextMessage(&bundle, &osc)) {
				tosc_printMessage(&osc);
			}
		} else {
			tosc_printOscBuffer(recvBuffer, recvsize);
		}
		recvaddr_size = sizeof(recvaddr);
	}

	close_socket(sock_fd);

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
