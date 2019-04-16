#include <stdio.h>		// for printf
#include <winsock2.h>	// the networking library
#pragma comment(lib, "ws2_32.lib")	// links WinSock2 with MS Visual Studio.
// with g++, use -lws2_32 as a command line arg to add the winsock 2 (32bit) library
#include <vector>	// for the server's client list and input buffer
#include <conio.h>	// for kbhit() and getch()

int serverLogic(SOCKET mySocket, sockaddr * connectionAddress);
int clientLogic(SOCKET mySocket, const sockaddr* connectionAddr);
int getReadStatus(const SOCKET a_socket);
int endBrokerSocket(SOCKET socket);
void finalWSACleanup();

int main()
{
	sockaddr_in connectionAddress;
	unsigned short myPort = 1337;
	// initialize the socket's address
	memset(&connectionAddress, 0, sizeof(sockaddr_in)); // initialize to zero
	connectionAddress.sin_family = AF_INET;
	// host-to-network-short: big-endian conversion of a 16 byte value
	connectionAddress.sin_port = htons(myPort);
	// initialize the server socket
	int result;
	WSADATA wsaData; // gets populated w/ info explaining this sockets implementation
	// load Winsock 2.0 DLL. initiates use of the Winsock DLL by a process
	if ((result = WSAStartup(MAKEWORD(1, 1),&wsaData)) != 0)
	{
		printf("WSAStartup() error %d\n", result);
		return EXIT_FAILURE;
	}
	atexit(finalWSACleanup); // add callback to trigger when program ends. cleans up sockets
	// create the main socket, either client or server
	SOCKET mySocket = socket(
/*
AF_UNSPEC: 0     The address family is unspecified.
AF_INET: 2       The Internet Protocol version 4 (IPv4) address family.
AF_IPX: 6        The IPX/SPX address family. This address family is only supported if the NWLink IPX/SPX NetBIOS Compatible Transport protocol is installed. This address family is not supported on Windows Vista and later.
AF_APPLETALK: 17 The AppleTalk address family. This address family is only supported if the AppleTalk protocol is installed. This address family is not supported on Windows Vista and later.
AF_NETBIOS: 17   The NetBIOS address family. This address family is only supported if a Windows Sockets provider for NetBIOS is installed.
AF_INET6: 23     The Internet Protocol version 6 (IPv6) address family.
AF_IRDA: 26      The Infrared Data Association (IrDA) address family. This address family is only supported if the computer has an infrared port and driver installed.
AF_BTH: 32       The Bluetooth address family. This address family is supported on Windows XP with SP2 or later if the computer has a Bluetooth adapter and driver installed.
*/
		AF_INET, 
/*
SOCK_STREAM: 1    A socket type that provides sequenced, reliable, two-way, connection-based byte streams with an OOB data transmission mechanism. This socket type uses the Transmission Control Protocol (TCP) for the Internet address family (AF_INET or AF_INET6).
SOCK_DGRAM: 2     A socket type that supports datagrams, which are connectionless, unreliable buffers of a fixed (typically small) maximum length. This socket type uses the User Datagram Protocol (UDP) for the Internet address family (AF_INET or AF_INET6).
SOCK_RAW: 3       A socket type that provides a raw socket that allows an application to manipulate the next upper-layer protocol header. To manipulate the IPv4 header, the IP_HDRINCL socket option must be set on the socket. To manipulate the IPv6 header, the IPV6_HDRINCL socket option must be set on the socket.
SOCK_RDM: 4       A socket type that provides a reliable message datagram. An example of this type is the Pragmatic General Multicast (PGM) multicast protocol implementation in Windows, often referred to as reliable multicast programming. This type is only supported if the Reliable Multicast Protocol is installed.
SOCK_SEQPACKET: 5 A socket type that provides a pseudo-stream packet based on datagrams.
*/
		SOCK_STREAM, 
/*
BTHPROTO_RFCOMM: 3 The Bluetooth Radio Frequency Communications (Bluetooth RFCOMM) protocol. This is a possible value when the af parameter is AF_BTH and the type parameter is SOCK_STREAM. This protocol is supported on Windows XP with SP2 or later.
IPPROTO_TCP: 6     The Transmission Control Protocol (TCP). This is a possible value when the af parameter is AF_INET or AF_INET6 and the type parameter is SOCK_STREAM.
IPPROTO_UDP: 17    The User Datagram Protocol (UDP). This is a possible value when the af parameter is AF_INET or AF_INET6 and the type parameter is SOCK_DGRAM.
IPPROTO_RM: 113    The PGM protocol for reliable multicast. This is a possible value when the af parameter is AF_INET and the type parameter is SOCK_RDM. On the Windows SDK released for Windows Vista and later, this value is also called IPPROTO_PGM. This protocol is only supported if the Reliable Multicast Protocol is installed.
*/
		IPPROTO_TCP);
	if (mySocket == INVALID_SOCKET)
	{
		printf("socket() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	// 1 to set non-blocking, 0 to set re-usable
	unsigned long argp = 1;
	// for complete info on this method, check out http://msdn.microsoft.com/en-us/library/ms740476(VS.85).aspx
	result = setsockopt(mySocket, 
		SOL_SOCKET,
/*
SO_REUSEADDR: 4 Allows the socket to be bound to an address that is already in use. For more information, see bind. Not applicable on ATM sockets.
*/
		SO_REUSEADDR, 
		(char *)&argp, sizeof(argp));
	if (result != 0)
	{
		printf("setsockopt() error %d\n", result);
		return EXIT_FAILURE;
	}
	// 1 to set non-blocking, 0 to set blocking
	argp = 1;
	// attempt to setup the socket as non-blocking
	if(ioctlsocket(mySocket, 
/*
FIONBIO:    The *argp parameter is a pointer to an unsigned long value. Set *argp to a nonzero value if the nonblocking mode should be enabled, or zero if the nonblocking mode should be disabled. When a socket is created, it operates in blocking mode by default (nonblocking mode is disabled). This is consistent with BSD sockets.
FIONREAD:   Use to determine the amount of data pending in the network's input buffer that can be read from socket s. The argp parameter points to an unsigned long value in which ioctlsocket stores the result. FIONREAD returns the amount of data that can be read in a single call to the recv function, which may not be the same as the total amount of data queued on the socket. If s is message oriented (for example, type SOCK_DGRAM), FIONREAD still returns the amount of pending data in the network buffer, however, the amount that can actually be read in a single call to the recv function is limited to the data size written in the send or sendto function call.
SIOCATMARK: Use to determine if all out of band (OOB) data has been read. (See Windows Sockets 1.1 Blocking Routines and EINPROGRESS for a discussion on OOB data.) This applies only to a stream oriented socket (for example, type SOCK_STREAM) that has been configured for in-line reception of any OOB data (SO_OOBINLINE). On sockets with the SO_OOBINLINE socket option set, SIOCATMARK always returns TRUE and the OOB data is returned to the user as normal data.
*/
			FIONBIO,
			&argp) == SOCKET_ERROR)
	{
		printf("ioctlsocket() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	// make the decision about client or server socket here.
	int userInput;
	do{
		printf("[c]lient or [s]erver?\n");
		userInput = _getch();
	}while(userInput != 'c' && userInput != 's');

	if(userInput == 's')
	{
		result = serverLogic(mySocket, (sockaddr*)&connectionAddress);
	}
	else
	{
		// connect to the server
		const char * targetIP = "127.0.0.1";
		unsigned long raw_ip_nbo = inet_addr(targetIP);
		if (raw_ip_nbo == INADDR_NONE)
		{
			printf("inet_addr() error \"%s\"\n", targetIP);
			return EXIT_FAILURE;
		}
		connectionAddress.sin_addr.s_addr = raw_ip_nbo;
		result = clientLogic(mySocket, (const sockaddr*)&connectionAddress);
	}
	if(result == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}
	if(mySocket != INVALID_SOCKET)
	{
		result = closesocket(mySocket); // server closes, it doesn't shut down
		if (result != 0)
		{
			printf("closesocket() error %d\n", WSAGetLastError());
			return EXIT_FAILURE;
		}
		mySocket = INVALID_SOCKET;
	}
	return EXIT_SUCCESS;
}

int serverLogic(SOCKET mySocket, sockaddr * connectionAddress)
{
	// start listening on the server
	int result = bind(mySocket, connectionAddress, sizeof(sockaddr_in));
	if (result == SOCKET_ERROR)
	{
		printf("bind() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	result = listen(mySocket, /* size of connection queue */10);
	if (result == SOCKET_ERROR)
	{
		printf("listen() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	// keep track of client connections
	struct ClientSocket {
		SOCKET socket;
		sockaddr address;
	};
	// used to broker connection with clients
	std::vector<ClientSocket> clientSockets;
	// keep track of messages read from clients
	std::vector<char> inputBuffer;
	// server loop
	int iterations = 0;
	while((result = getReadStatus(mySocket)) != SOCKET_ERROR)
	{
		if(_kbhit() && _getch() == 27)	// escape key press
		{
			break; // end the server loop
		}
		if(result == 1)
		{
			printf("connecting client...");
			int client_size = sizeof(sockaddr_in);
			ClientSocket clientSocket;
			clientSocket.socket = accept(mySocket, 
				(struct sockaddr*)&clientSocket.address, &client_size);
			if(clientSocket.socket == INVALID_SOCKET)
			{
				printf("accept() error %d\n", WSAGetLastError());
				return EXIT_FAILURE;
			}
			clientSockets.push_back(clientSocket);
			printf("\rconnected client %d\n", clientSockets.size());
		}
		else
		{
			printf("server: %d\r", iterations++);
			// read all the data from the clients
			inputBuffer.clear();
			for(unsigned int i = 0; i < clientSockets.size(); ++i)
			{
				int status = getReadStatus(clientSockets[i].socket);
				if(status == 1)
				{
					if(clientSockets[i].socket == INVALID_SOCKET)
					{
						continue;
					}
					unsigned long howMuchInBuffer = 0;
					ioctlsocket(clientSockets[i].socket, FIONREAD, 
						&howMuchInBuffer);
					if(howMuchInBuffer > 0)
					{
						unsigned long used = inputBuffer.size();
						inputBuffer.resize(used + howMuchInBuffer, 0);
						result = recv(clientSockets[i].socket, 
							(char*)(&inputBuffer[used]),howMuchInBuffer,0);
						// NOTE: for multi-byte values sent across the network
						// ntohl or ntohs should be used to convert 4-byte and
						// 2-byte values respectively.
						if(result == SOCKET_ERROR)
						{
							printf("client %d recv() error %d\n", i, 
								WSAGetLastError());
							return EXIT_FAILURE;
						}
					}
				}
				else if(status == SOCKET_ERROR)
				{
					printf("\rclient %d read error\n", i);
					int result = endBrokerSocket(clientSockets[i].socket);
					if(result == EXIT_FAILURE)
					{
						return EXIT_FAILURE;
					}
					clientSockets[i].socket = INVALID_SOCKET;
				}
			}
			if(inputBuffer.size() > 0)
			{
				// print data about to be sent
				printf("sending: \"");
				for(unsigned int i = 0; i < inputBuffer.size(); ++i)
				{
					printf("%c", inputBuffer[i]);
				}
				printf("\" %d bytes\n", inputBuffer.size());
				// send all known data to all clients
				for(unsigned int i = 0; i < clientSockets.size(); ++i)
				{
					if(clientSockets[i].socket == INVALID_SOCKET)
					{
						continue;
					}
					// NOTE: for multi-byte values sent across the network
					// htonl or htons should be used to convert 4-byte and
					// 2-byte values respectively.
					result = send(clientSockets[i].socket, 
						(const char*)&inputBuffer[0],inputBuffer.size(),0);
					if(result == SOCKET_ERROR) // if the socket failed
					{
						printf("\rclient %d send error %d\n", i, 
							WSAGetLastError());
						int result = endBrokerSocket(clientSockets[i].socket);
						if(result == EXIT_FAILURE)
						{
							return EXIT_FAILURE;
						}
						clientSockets[i].socket = INVALID_SOCKET;
					}
				}
			}
			// removed released sockets from list, backwards to avoid skips
			if(clientSockets.size() > 0)
			{
				for(int i = (signed)clientSockets.size()-1; i >= 0; --i)
				{
					if(clientSockets[i].socket == INVALID_SOCKET)
					{
						clientSockets.erase(clientSockets.begin()+i);
					}
				}
			}
		}
	}
	// release all client sockets
	for(unsigned int i = 0; i < clientSockets.size(); ++i)
	{
		int result = endBrokerSocket(clientSockets[i].socket);
		if(result == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}
		clientSockets[i].socket = INVALID_SOCKET;
	}
	return EXIT_SUCCESS;
}

int endBrokerSocket(SOCKET socket)
{
	int result = shutdown(socket, SD_BOTH);
	if (result != 0)
	{
		printf("socket shutdown() error %d\n", WSAGetLastError());
	}
	result = closesocket(socket);
	if (result != 0)
	{
		printf("socket closesocket() error %d\n", WSAGetLastError());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int clientLogic(SOCKET mySocket, const sockaddr* connectionAddress)
{
	int result = connect(mySocket, connectionAddress, sizeof(sockaddr_in));
	if(result == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		switch(errorCode)
		{
		case WSAEISCONN:	printf("already connected!\n");	break;
		case WSAEWOULDBLOCK:
		case WSAEALREADY:
			break;
		default:
			printf("client connect() error %d\n", errorCode);
			return EXIT_FAILURE;
		}
	}
	// client loop
	int iterations = 0;
	bool sendit = false;
	char userTextField[1024];
	userTextField[0] = '\0';
	int userTextFieldCursor = 0;
	int userInput;
	while(getReadStatus(mySocket) != SOCKET_ERROR)
	{
		// if user types, remember what's being typed, send with enter.
		if(_kbhit())
		{
			userInput = _getch();
			sendit = userInput == '\n' || userInput == '\r';
			if(!sendit)
			{
				putchar(userInput);
				userTextField[userTextFieldCursor++] = userInput;
				userTextField[userTextFieldCursor] = '\0';
				if(userTextFieldCursor >= sizeof(userTextField)-1)
				{
					sendit = true;
				}
			}
			if(sendit)
			{
				// NOTE: for multi-byte values sent across the network
				// htonl or htons should be used to convert 4-byte and
				// 2-byte values respectively.
				result = send(mySocket,
					(const char *)userTextField, userTextFieldCursor, 0);
				if(result == SOCKET_ERROR)
				{
					printf("client send() error %d\n", WSAGetLastError());
					return EXIT_FAILURE;
				}
				userTextFieldCursor = 0;
				userTextField[userTextFieldCursor] = '\0';
			}
		}
		// receive data from the server, if there is any
		if(getReadStatus(mySocket) == 1)
		{
			unsigned long howMuchInBuffer = 0;
			unsigned long numBytesRead = 0;
			printf("received: \"");
			do
			{
				ioctlsocket(mySocket, FIONREAD, &howMuchInBuffer);
				// 4 bytes at a time out of the socket, stored in userInput
				int result = recv(mySocket,
					(char*)(&userInput), sizeof(userInput), 0);
				// NOTE: for multi-byte values sent across the network
				// ntohl or ntohs should be used to convert 4-byte and
				// 2-byte values respectively.
				if(result == SOCKET_ERROR)
				{
					printf("client recv() error %d\n", WSAGetLastError());
					return EXIT_FAILURE;
				}
				for(int i = 0; i < result; ++i)
				{
					printf("%c",  ((char*)(&userInput))[i]);
					numBytesRead++;
				}
				howMuchInBuffer -= result;
			}
			while(howMuchInBuffer > 0);
			printf("\" %d bytes%c", numBytesRead, 
				((numBytesRead!=0)?'\n':'\r'));
		}
		else
		{
			printf("client: \"%s\" %d\r", userTextField, iterations++);
		}
	}
	return EXIT_SUCCESS;
}

int getReadStatus(const SOCKET a_socket)
{
	// zero seconds, zero milliseconds. max time select call allowd to block
	static const timeval instantSpeedPlease = {0,0};
	fd_set a = {1, {a_socket}};
/*
select returns the number of ready socket handles in the fd_set structure, zero if the time limit expired, or SOCKET_ERROR if an error occurred. WSAGetLastError can be used to retrieve a specific error code.
*/
	int result = select(0,&a,0,0,&instantSpeedPlease);
	if(result == SOCKET_ERROR)
	{
		result = WSAGetLastError();
	}
	if(result != 0 && result != 1)
	{
		printf("select() error %d\n", result);
		return SOCKET_ERROR;
	}
	return result;
}

void finalWSACleanup() // callback used to clean up sockets
{
	int result = WSACleanup();
	if (result != 0)
	{
		printf("WSACleanup() error %d\n", result);
	}
}