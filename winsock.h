#pragma once
#include "common.h" 

// the .h file defines all windows socket functions 

class Winsock
{
public:

	static void initialize()   // call Winsock::intialize() in main, to intialize winsock only once
	{
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);   // defined in winsock2.h
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			WSACleanup();
			exit(1); // quit program
		}
	}

	static void cleanUp() // call Winsock::cleanUp() in main only once
	{
		WSACleanup();
	}

	int createTCPSocket(void)
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET) {
			printf("socket() error %d\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		return 0;
	}

	// host (e.g., "www.google.com" or "132.145.2.1"), return its corresponding IP address of type DWORD
	DWORD getIPaddress(string host)
	{
		struct sockaddr_in server; // structure for connecting to server
		struct hostent *remote;    // structure used in DNS lookups: convert host name into IP address

		// first assume that the string is an IP address
		DWORD IP = inet_addr(host.c_str());
		if (IP == INADDR_NONE)
		{
			if ((remote = gethostbyname(host.c_str())) == NULL)
			{
				printf("Invalid host name string: not FQDN\n");
				return INADDR_NONE;  // 1 means failed
			}
			else // take the first IP address and copy into sin_addr
			{
				memcpy((char *)&(server.sin_addr), remote->h_addr, remote->h_length);
				IP = server.sin_addr.S_un.S_addr;
			}
		}
		//	printf("Server %s (IP: %s)\n", host.c_str(), inet_ntoa(server.sin_addr));  // for debugging purpose
		return IP;
	}

	// host IP address in binary version, port: 2-byte short
	int connectToServerIP(DWORD IP, short port)
	{
		if (IP == INADDR_NONE)
		{
			printf("Invalid IP address\n");
			return 1;  // 1 means error
		}
		struct sockaddr_in server; // structure for connecting to server
		server.sin_addr.S_un.S_addr = IP; // irectly drop its binary version into sin_addr

		// setup the port # and protocol type
		server.sin_family = AF_INET;   // IPv4
		server.sin_port = htons(port); // host-to-network flips the byte order

		if (connect(sock, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			printf("Connection error: %d\n", WSAGetLastError());
			return 1;
		}
		printf("Successfully connected to %s on port %d\n", inet_ntoa(server.sin_addr), htons(server.sin_port));

		return 0;
	}

	// define your sendRequest(...) function, to send a HEAD or GET request

	// define your receive(...) function, to receive the reply from the server

	void closeSocket(void)
	{
		closesocket(sock);
	}



private:
	SOCKET sock;

	// define other private variables if needed

};