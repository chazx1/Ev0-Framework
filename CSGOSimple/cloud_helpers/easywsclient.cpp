#ifdef _WIN32
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // _CRT_SECURE_NO_WARNINGS for sscanf errors in MSVC2013 Express
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <memory>
#include <fcntl.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "crypt32")
#pragma comment( lib, "ws2_32" )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <io.h>
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef _SOCKET_T_DEFINED
typedef SOCKET socket_t;
#define _SOCKET_T_DEFINED
#endif
#ifndef snprintf
#define snprintf _snprintf_s
#endif
#if _MSC_VER >=1600
// vs2010 or later
#include <stdint.h>
#include <stdlib.h>
#else
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
#define socketerrno WSAGetLastError()
#define SOCKET_EAGAIN_EINPROGRESS WSAEINPROGRESS
#define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#ifndef _SOCKET_T_DEFINED
typedef int socket_t;
#define _SOCKET_T_DEFINED
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif
#define closesocket(s) ::close(s)
#include <errno.h>
#define socketerrno errno
#define SOCKET_EAGAIN_EINPROGRESS EAGAIN
#define SOCKET_EWOULDBLOCK EWOULDBLOCK
#endif

#include <vector>
#include <string>
#include <cassert>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "easywsclient.hpp"

#pragma comment( lib, "libcrypto.lib" )
#pragma comment( lib, "libssl.lib" )

using easywsclient::Callback_Imp;
using easywsclient::BytesCallback_Imp;


namespace { // private module-only namespace
	class isocket {
	public:
		virtual bool is_invalid() = 0;
		virtual ssize_t send(const void *buf, size_t len, int flags) = 0;
		virtual ssize_t recv(void *buf, size_t len, int flags) = 0;
		virtual void set_options() = 0;
		virtual void close() = 0;
		virtual void set_timeout(int timeout, bool have_buf) = 0;
		virtual bool is_ssl() = 0;
	};

	typedef std::shared_ptr<isocket> isocket_ptr;

	class raw_socket : public isocket {
	public:
		raw_socket(socket_t s) :m_socket(s) {}
		virtual bool is_invalid() {
			return m_socket == INVALID_SOCKET;
		}
		virtual ssize_t send(const void *buf, size_t len, int flags) {
			return ::send(m_socket, (const char*)buf, len, flags);
		}
		virtual ssize_t recv(void *buf, size_t len, int flags) {
			return ::recv(m_socket, (char*)buf, len, flags);
		}
		virtual void set_options() {
			// Disable Nagle's algorithm
			int flag = 1;
			setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
#ifdef _WIN32
			u_long on = 1;
			ioctlsocket(m_socket, FIONBIO, &on);
#else
			fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif
		}
		virtual void close() {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
		virtual void set_timeout(int timeout, bool have_buf) {
			fd_set rfds;
			fd_set wfds;
			timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
			FD_ZERO(&rfds);
			FD_ZERO(&wfds);
			FD_SET(m_socket, &rfds);
			if (have_buf) { FD_SET(m_socket, &wfds); }
			select(m_socket + 1, &rfds, &wfds, 0, timeout > 0 ? &tv : 0);
		}
		virtual bool is_ssl() {
			return false; 
		}
		virtual ~raw_socket() {
			close();
		}
	private:
		socket_t m_socket;
	};

	class ssl_socket : public raw_socket {
	public:
		ssl_socket(socket_t s) :raw_socket(s) {
			const SSL_METHOD *meth = SSLv23_method();
			ctx = SSL_CTX_new(meth); 
			ssl = SSL_new(ctx); 
			SSL_set_fd(ssl, s);
			SSL_connect(ssl);
		}
		virtual ssize_t send(const void *buf, size_t len, int) {
			return SSL_write(ssl, buf, len);
		}
		virtual ssize_t recv(void *buf, size_t len, int) {
			return SSL_read(ssl, buf, len);
		}
		virtual bool is_ssl() {
			return true;
		}
		virtual ~ssl_socket() {
			if (ssl)
				SSL_free(ssl);
			if (ssl)
				SSL_CTX_free(ctx);
		}
	private:
		SSL * ssl = nullptr;
		SSL_CTX *ctx = nullptr;
	};

	isocket_ptr hostname_connect(const std::string& hostname, int port, bool is_ssl) {
		struct addrinfo hints;
		struct addrinfo *result;
		struct addrinfo *p;
		int ret;
		socket_t sockfd = INVALID_SOCKET;
		char sport[16];
		memset(&hints, 0, sizeof(hints));
 		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		snprintf(sport, 16, "%d", port);
		if ((ret = getaddrinfo(hostname.c_str(), sport, &hints, &result)) != 0)
		{
			//fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(ret));
			return isocket_ptr();
		}
		for (p = result; p != NULL; p = p->ai_next)
		{
			sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (sockfd == INVALID_SOCKET) { continue; }
			if (connect(sockfd, p->ai_addr, p->ai_addrlen) != SOCKET_ERROR) {
				break;
			}
			closesocket(sockfd);
			sockfd = INVALID_SOCKET;
		}
		freeaddrinfo(result);
		if (is_ssl == false) {
			return isocket_ptr(new raw_socket(sockfd));
		}
		else {
			return isocket_ptr(new ssl_socket(sockfd));
		}
		return isocket_ptr();
	}


	class _DummyWebSocket : public easywsclient::WebSocket
	{
	public:
		void poll(int timeout) { }
		void send(const std::string& message) { }
		void sendBinary(const std::string& message) { }
		void sendBinary(const std::vector<uint8_t>& message) { }
		void sendPing() { }
		void close() { }
		bool isSSL() { return false; }
		readyStateValues getReadyState() const { return CLOSED; }
		void _dispatch(Callback_Imp & callable) { }
		void _dispatchBinary(BytesCallback_Imp& callable) { }
	};


	class _RealWebSocket : public easywsclient::WebSocket
	{
	public:
		// http://tools.ietf.org/html/rfc6455#section-5.2  Base Framing Protocol
		//
		//  0                   1                   2                   3
		//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		// +-+-+-+-+-------+-+-------------+-------------------------------+
		// |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
		// |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
		// |N|V|V|V|       |S|             |   (if payload len==126/127)   |
		// | |1|2|3|       |K|             |                               |
		// +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
		// |     Extended payload length continued, if payload len == 127  |
		// + - - - - - - - - - - - - - - - +-------------------------------+
		// |                               |Masking-key, if MASK set to 1  |
		// +-------------------------------+-------------------------------+
		// | Masking-key (continued)       |          Payload Data         |
		// +-------------------------------- - - - - - - - - - - - - - - - +
		// :                     Payload Data continued ...                :
		// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
		// |                     Payload Data continued ...                |
		// +---------------------------------------------------------------+
		struct wsheader_type {
			unsigned header_size;
			bool fin;
			bool mask;
			enum opcode_type {
				CONTINUATION = 0x0,
				TEXT_FRAME = 0x1,
				BINARY_FRAME = 0x2,
				CLOSE = 8,
				PING = 9,
				PONG = 0xa,
			} opcode;
			int N0;
			uint64_t N;
			uint8_t masking_key[4];
		};

		std::vector<uint8_t> rxbuf;
		std::vector<uint8_t> txbuf;
		std::vector<uint8_t> receivedData;

		isocket_ptr isockfd;
		readyStateValues readyState;
		bool useMask;

		_RealWebSocket(isocket_ptr sockfd, bool useMask) : isockfd(sockfd), readyState(OPEN), useMask(useMask) {
		}

		bool isSSL() {
			return isockfd ? isockfd->is_ssl() : false;
		}

		readyStateValues getReadyState() const {
			return readyState;
		}

		void poll(int timeout) { // timeout in milliseconds
			if (readyState == CLOSED) {
				if (timeout > 0) {
					timeval tv = { timeout / 1000, (timeout % 1000) * 1000 };
					select(0, NULL, NULL, NULL, &tv);
				}
				return;
			}
			if (timeout != 0) {
				isockfd->set_timeout(timeout, txbuf.size());
			}
			while (true) {
				// FD_ISSET(0, &rfds) will be true
				int N = rxbuf.size();
				ssize_t ret;
				rxbuf.resize(N + 1500);
				ret = isockfd->recv((char*)&rxbuf[0] + N, 1500, 0);
				if (false) {}
				else if (ret < 0 && (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)) {
					rxbuf.resize(N);
					break;
				}
				else if (ret <= 0) {
					rxbuf.resize(N);
					readyState = CLOSED;
					fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stdout);
					break;
				}
				else {
					rxbuf.resize(N + ret);
				}
			}
			while (txbuf.size()) {
				int ret = isockfd->send((char*)&txbuf[0], txbuf.size(), 0);
				if (false) {} // ??
				else if (ret < 0 && (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)) {
					break;
				}
				else if (ret <= 0) {
					isockfd->close();
					readyState = CLOSED;
					fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stdout);
					break;
				}
				else {
					txbuf.erase(txbuf.begin(), txbuf.begin() + ret);
				}
			}
			if (!txbuf.size() && readyState == CLOSING) {
				isockfd->close();
				readyState = CLOSED;
			}
		}

		// Callable must have signature: void(const std::string & message).
		// Should work with C functions, C++ functors, and C++11 std::function and
		// lambda:
		//template<class Callable>
		//void dispatch(Callable callable)
		virtual void _dispatch(Callback_Imp & callable) {
			struct CallbackAdapter : public BytesCallback_Imp
				// Adapt void(const std::string<uint8_t>&) to void(const std::string&)
			{
				Callback_Imp& callable;
				CallbackAdapter(Callback_Imp& callable) : callable(callable) { }
				void operator()(const std::vector<uint8_t>& message) {
					std::string stringMessage(message.begin(), message.end());
					callable(stringMessage);
				}
			};
			CallbackAdapter bytesCallback(callable);
			_dispatchBinary(bytesCallback);
		}

		virtual void _dispatchBinary(BytesCallback_Imp & callable) {
			// TODO: consider acquiring a lock on rxbuf...
			while (true) {
				wsheader_type ws;
				if (rxbuf.size() < 2) { return; /* Need at least 2 */ }
				const uint8_t * data = (uint8_t *)&rxbuf[0]; // peek, but don't consume
				ws.fin = (data[0] & 0x80) == 0x80;
				ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
				ws.mask = (data[1] & 0x80) == 0x80;
				ws.N0 = (data[1] & 0x7f);
				ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 8 : 0) + (ws.mask ? 4 : 0);
				if (rxbuf.size() < ws.header_size) { return; /* Need: ws.header_size - rxbuf.size() */ }
				int i = 0;
				if (ws.N0 < 126) {
					ws.N = ws.N0;
					i = 2;
				}
				else if (ws.N0 == 126) {
					ws.N = 0;
					ws.N |= ((uint64_t)data[2]) << 8;
					ws.N |= ((uint64_t)data[3]) << 0;
					i = 4;
				}
				else if (ws.N0 == 127) {
					ws.N = 0;
					ws.N |= ((uint64_t)data[2]) << 56;
					ws.N |= ((uint64_t)data[3]) << 48;
					ws.N |= ((uint64_t)data[4]) << 40;
					ws.N |= ((uint64_t)data[5]) << 32;
					ws.N |= ((uint64_t)data[6]) << 24;
					ws.N |= ((uint64_t)data[7]) << 16;
					ws.N |= ((uint64_t)data[8]) << 8;
					ws.N |= ((uint64_t)data[9]) << 0;
					i = 10;
				}
				if (ws.mask) {
					ws.masking_key[0] = ((uint8_t)data[i + 0]) << 0;
					ws.masking_key[1] = ((uint8_t)data[i + 1]) << 0;
					ws.masking_key[2] = ((uint8_t)data[i + 2]) << 0;
					ws.masking_key[3] = ((uint8_t)data[i + 3]) << 0;
				}
				else {
					ws.masking_key[0] = 0;
					ws.masking_key[1] = 0;
					ws.masking_key[2] = 0;
					ws.masking_key[3] = 0;
				}
				if (rxbuf.size() < ws.header_size + ws.N) { return; /* Need: ws.header_size+ws.N - rxbuf.size() */ }

				// We got a whole message, now do something with it:
				if (false) {}
				else if (
					ws.opcode == wsheader_type::TEXT_FRAME
					|| ws.opcode == wsheader_type::BINARY_FRAME
					|| ws.opcode == wsheader_type::CONTINUATION
					) {
					if (ws.mask) { for (size_t i = 0; i != ws.N; ++i) { rxbuf[i + ws.header_size] ^= ws.masking_key[i & 0x3]; } }
					receivedData.insert(receivedData.end(), rxbuf.begin() + ws.header_size, rxbuf.begin() + ws.header_size + (size_t)ws.N);// just feed
					if (ws.fin) {
						callable((const std::vector<uint8_t>) receivedData);
						receivedData.erase(receivedData.begin(), receivedData.end());
						std::vector<uint8_t>().swap(receivedData);// free memory
					}
				}
				else if (ws.opcode == wsheader_type::PING) {
					if (ws.mask) { for (size_t i = 0; i != ws.N; ++i) { rxbuf[i + ws.header_size] ^= ws.masking_key[i & 0x3]; } }
					std::string data(rxbuf.begin() + ws.header_size, rxbuf.begin() + ws.header_size + (size_t)ws.N);
					sendData(wsheader_type::PONG, data.size(), data.begin(), data.end());
				}
				else if (ws.opcode == wsheader_type::PONG) {}
				else if (ws.opcode == wsheader_type::CLOSE) { close(); }
				else { fprintf(stdout, "ERROR: Got unexpected WebSocket message.\n"); close(); }

				rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size + (size_t)ws.N);
			}
		}

		void sendPing() {
			std::string empty;
			sendData(wsheader_type::PING, empty.size(), empty.begin(), empty.end());
		}

		void send(const std::string& message) {
			sendData(wsheader_type::TEXT_FRAME, message.size(), message.begin(), message.end());
		}

		void sendBinary(const std::string& message) {
			sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
		}

		void sendBinary(const std::vector<uint8_t>& message) {
			sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
		}

		template<class Iterator>
		void sendData(wsheader_type::opcode_type type, uint64_t message_size, Iterator message_begin, Iterator message_end) {
			// TODO:
			// Masking key should (must) be derived from a high quality random
			// number generator, to mitigate attacks on non-WebSocket friendly
			// middleware:
			const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
			// TODO: consider acquiring a lock on txbuf...
			if (readyState == CLOSING || readyState == CLOSED) { return; }
			std::vector<uint8_t> header;
			header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (useMask ? 4 : 0), 0);
			header[0] = 0x80 | type;
			if (false) {}
			else if (message_size < 126) {
				header[1] = (message_size & 0xff) | (useMask ? 0x80 : 0);
				if (useMask) {
					header[2] = masking_key[0];
					header[3] = masking_key[1];
					header[4] = masking_key[2];
					header[5] = masking_key[3];
				}
			}
			else if (message_size < 65536) {
				header[1] = 126 | (useMask ? 0x80 : 0);
				header[2] = (message_size >> 8) & 0xff;
				header[3] = (message_size >> 0) & 0xff;
				if (useMask) {
					header[4] = masking_key[0];
					header[5] = masking_key[1];
					header[6] = masking_key[2];
					header[7] = masking_key[3];
				}
			}
			else { // TODO: run coverage testing here
				header[1] = 127 | (useMask ? 0x80 : 0);
				header[2] = (message_size >> 56) & 0xff;
				header[3] = (message_size >> 48) & 0xff;
				header[4] = (message_size >> 40) & 0xff;
				header[5] = (message_size >> 32) & 0xff;
				header[6] = (message_size >> 24) & 0xff;
				header[7] = (message_size >> 16) & 0xff;
				header[8] = (message_size >> 8) & 0xff;
				header[9] = (message_size >> 0) & 0xff;
				if (useMask) {
					header[10] = masking_key[0];
					header[11] = masking_key[1];
					header[12] = masking_key[2];
					header[13] = masking_key[3];
				}
			}
			// N.B. - txbuf will keep growing until it can be transmitted over the socket:
			txbuf.insert(txbuf.end(), header.begin(), header.end());
			txbuf.insert(txbuf.end(), message_begin, message_end);
			if (useMask) {
				for (size_t i = 0; i != message_size; ++i) { *(txbuf.end() - message_size + i) ^= masking_key[i & 0x3]; }
			}
		}

		void close() {
			if (readyState == CLOSING || readyState == CLOSED) { return; }
			readyState = CLOSING;
			uint8_t closeFrame[6] = { 0x88, 0x80, 0x00, 0x00, 0x00, 0x00 }; // last 4 bytes are a masking key
			std::vector<uint8_t> header(closeFrame, closeFrame + 6);
			txbuf.insert(txbuf.end(), header.begin(), header.end());
		}

	};


	easywsclient::WebSocket::pointer from_url(const std::string& url, bool useMask, const std::string& origin) {
		const int max_size_buff = 512;
		char host[max_size_buff];
		int port = 0;
		char path[max_size_buff];
		if (url.size() >= max_size_buff) {
			fprintf(stdout, "ERROR: url size limit exceeded: %s\n", url.c_str());
			return NULL;
		}
		if (origin.size() >= 200) {
			fprintf(stdout, "ERROR: origin size limit exceeded: %s\n", origin.c_str());
			return NULL;
		}
		bool is_ssl = true;
		bool default_port = false;
		if (sscanf(url.c_str(), "wss://%[^:/]:%d/%s", host, &port, path) == 3) {

		}
		else if (sscanf(url.c_str(), "wss://%[^:/]/%s", host, path) == 2) {
			port = 443;
		}
		else if (sscanf(url.c_str(), "wss://%[^:/]:%d", host, &port) == 2) {
			path[0] = '\0';
		}
		else if (sscanf(url.c_str(), "wss://%[^:/]", host) == 1) {
			port = 443;
			path[0] = '\0';
		}
		default_port = port == 443;
		if (port == 0) {
			is_ssl = false;
			if (sscanf(url.c_str(), "ws://%[^:/]:%d/%s", host, &port, path) == 3) {

			}
			else if (sscanf(url.c_str(), "ws://%[^:/]/%s", host, path) == 2) {
				port = 80;
			}
			else if (sscanf(url.c_str(), "ws://%[^:/]:%d", host, &port) == 2) {
				path[0] = '\0';
			}
			else if (sscanf(url.c_str(), "ws://%[^:/]", host) == 1) {
				port = 80;
				path[0] = '\0';
			}
			else {
				fprintf(stdout, "ERROR: Could not parse WebSocket url: %s\n", url.c_str());
				return NULL;
			}
			default_port = port == 80;
		}
		fprintf(stdout, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, port, path);
		isocket_ptr sockfd = hostname_connect(host, port, is_ssl);
		if (!sockfd || sockfd->is_invalid()) {
			fprintf(stdout, "Unable to connect to %s:%d\n", host, port);
			return NULL;
		}
		{
			// XXX: this should be done non-blocking,
			char line[max_size_buff];
			int status;
			int i;
			snprintf(line, max_size_buff, "GET /%s HTTP/1.1\r\n", path); sockfd->send(line, strlen(line), 0);
			if (default_port) {
				snprintf(line, max_size_buff, "Host: %s\r\n", host); sockfd->send(line, strlen(line), 0);
			}
			else {
				snprintf(line, max_size_buff, "Host: %s:%d\r\n", host, port); sockfd->send(line, strlen(line), 0);
			}
			snprintf(line, max_size_buff, "Upgrade: websocket\r\n"); sockfd->send(line, strlen(line), 0);
			snprintf(line, max_size_buff, "Connection: Upgrade\r\n"); sockfd->send(line, strlen(line), 0);
			snprintf(line, max_size_buff, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36 Edge/16.16299\r\n"); sockfd->send(line, strlen(line), 0);
			snprintf(line, max_size_buff, "Cache-Control: no-cache\r\n"); sockfd->send(line, strlen(line), 0);
			if (!origin.empty()) {
				snprintf(line, max_size_buff, "Origin: %s\r\n", origin.c_str()); sockfd->send(line, strlen(line), 0);
			}
			snprintf(line, max_size_buff, "Sec-WebSocket-Key: iKo6WW+gYbhK5WQj4DlqXw==\r\n"); sockfd->send(line, strlen(line), 0);
			snprintf(line, max_size_buff, "Sec-WebSocket-Version: 13\r\n"); sockfd->send(line, strlen(line), 0);
			snprintf(line, max_size_buff, "\r\n"); sockfd->send(line, strlen(line), 0);
			for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i) {
				if (sockfd->recv(line + i, 1, 0) == 0) { 
					return NULL;
				}
			}
			line[i] = 0;
			if (i == max_size_buff) { fprintf(stdout, "ERROR: Got invalid status line connecting to: %s\n", url.c_str()); return NULL; }
			if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101) { fprintf(stdout, "ERROR: Got bad status connecting to %s: %s", url.c_str(), line); return NULL; }
			// TODO: verify response headers,
			while (true) {
				for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i) { if (sockfd->recv(line + i, 1, 0) == 0) { return NULL; } }
				if (line[0] == '\r' && line[1] == '\n') { break; }
			}
		}
		sockfd->set_options();
		fprintf(stdout, "Connected to: %s\n", url.c_str());
		return easywsclient::WebSocket::pointer(new _RealWebSocket(sockfd, useMask));
	}

} // end of module-only namespace



namespace easywsclient {

	WebSocket::pointer WebSocket::create_dummy() {
		static pointer dummy = pointer(new _DummyWebSocket);
		return dummy;
	}


	WebSocket::pointer WebSocket::from_url(const std::string& url, const std::string& origin) {
		return ::from_url(url, true, origin);
	}

	WebSocket::pointer WebSocket::from_url_no_mask(const std::string& url, const std::string& origin) {
		return ::from_url(url, false, origin);
	}


} // namespace easywsclient
