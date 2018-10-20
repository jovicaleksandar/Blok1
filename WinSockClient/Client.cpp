#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016

// Initializes WinSock2 library
// Returns true if succeeded, false otherwise.
bool InitializeWindowsSockets();
int SendPacket(SOCKET *socket, char * message, int messageSize);
int RecievePacket(SOCKET *socket, char * recvBuffer, int length);
int SendMessage(char *queueName, void *message, int messageSize);
int Select(SOCKET socket, int recvSend);
SOCKET connectSocket;

int __cdecl main(int argc, char **argv) 
{
    // socket used to communicate with server
    connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // message to send
    char *messageToSend = "this is a test";
    
    // Validate the parameters
    if (argc != 2)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(DEFAULT_PORT);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

	unsigned long mode = 1; //non-blocking mode
	iResult = ioctlsocket(connectSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", iResult);
	}
 
    // Send an prepared message with null terminator included
    /*iResult = send( connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0 );

    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);*/

	do
	{
		char* queueName = (char*)malloc(100);
		printf("Type queue name:");
		fflush(stdin);
		fgets(queueName, 100, stdin);

		char* message = (char*)malloc(100);
		printf("Write message to send: ");
		fflush(stdin);
		fgets(message, 100, stdin);

		int duzina = strlen(message) + strlen(queueName) + 1;

		//int messageSize = strlen(message);

		iResult = SendPacket(&connectSocket, (char*)&duzina, sizeof(int));
		iResult = SendMessage(queueName, message, duzina);

		//iResult = SendPacket(&connectSocket, message, messageSize);

		free (queueName);
		free(message);

		//************************************************************************************************************************

		int len;
		iResult = RecievePacket(&connectSocket, (char*)&len, 4);

		if (iResult != 1)
		{
			if (iResult == -1)
			{
				printf("Recived more bytes than expected.\n");
			}
			else
			{
				printf("Error while getting packets. Error code: %d\n", iResult);
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
		}

		printf("Length: %d\n", len);
		char *recvBuffer = (char*)malloc(len + 1);
		memset(recvBuffer, 0, 1);

		iResult = RecievePacket(&connectSocket, recvBuffer, len);

		if (iResult != 1)
		{
			if (iResult == -1)
			{
				printf("Recived more bytes than expected.\n");
			}
			else
			{
				printf("Error while getting packets. Error code: %d\n", iResult);
				closesocket(connectSocket);
				WSACleanup();
				return 1;
			}
		}

		recvBuffer[len] = 0;
		printf("Message recived: %s\n", recvBuffer);
		free(recvBuffer);


	} while (true);

	getchar();

    // cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
	// Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
	return true;
}

int SendMessage(char *queueName, void *message, int messageSize)
{
	queueName[strlen(queueName)-1] = '|';
	int poslao = 0;
	int msgSize = messageSize;
	char *temp = (char*)malloc(100);
	memset(temp, 0, 1);
	strcat(temp, queueName);
	strcat(temp, (char*)message);
	message = temp;

	do
	{
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(connectSocket, &writefds);

		timeval timeVal;
		timeVal.tv_sec = 1;
		timeVal.tv_usec = 0;
		FD_SET(connectSocket, &writefds);

		int result = select(0, NULL, &writefds, NULL, &timeVal);
		if (result > 0)
		{
			if (FD_ISSET(connectSocket, &writefds))
			{
				int iResult = send(connectSocket, (char*)message + poslao, messageSize - poslao, 0);
				if (iResult == SOCKET_ERROR)
				{
					return WSAGetLastError();
				}

				poslao += iResult;
				msgSize -= iResult;
				if (msgSize < 0)
				{
					return -1;
				}
			}
		}
		FD_CLR(connectSocket, &writefds);
	} while (msgSize > 0);

	return 1;
}


int SendPacket(SOCKET *socket, char * message, int messageSize)
{
	int poslao = 0;
	int msgSize = messageSize;
	do
	{
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(*socket, &writefds);

		timeval timeVal;
		timeVal.tv_sec = 1;
		timeVal.tv_usec = 0;

		FD_SET(*socket, &writefds);
		int result = select(0, NULL, &writefds, NULL, &timeVal);
		if (result > 0)
		{
			if (FD_ISSET(*socket, &writefds))
			{
				int iResult = send(*socket, message + poslao, messageSize - poslao, 0);
				if (iResult == SOCKET_ERROR)
				{
					return WSAGetLastError();
				}

				poslao += iResult;
				msgSize -= iResult;
				if (msgSize < 0)
				{
					return -1;
				}
			}
		}
		FD_CLR(*socket, &writefds);
	} while (msgSize > 0);

	return 1;
}

int RecievePacket(SOCKET *socket, char * recvBuffer, int length)
{
	int primio = 0;
	int len = length;

	do
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(*socket, &readfds);

		timeval timeVal;
		timeVal.tv_sec = 1;
		timeVal.tv_usec = 0;

		FD_SET(*socket, &readfds);
		int result = select(0, &readfds, NULL, NULL, &timeVal);
		if (result > 0)
		{
			if (FD_ISSET(*socket, &readfds))
			{
				int iResult = recv(*socket, recvBuffer + primio, length - primio, 0);
				primio += iResult;
				if (iResult > 0)
				{
					//printf("Message %s.\n", recvBuffer);
				}
				else if (iResult == 0)
				{
					// connection was closed gracefully
					printf("Connection with client closed.\n");
					closesocket(*socket);
				}
				else
				{
					// there was an error during recv
					printf("recv failed with error: %d\n", WSAGetLastError());
					return WSAGetLastError();
				}
				len -= iResult;
				if (len < 0)
				{
					printf("Greska primio vise nego sto treba.");
					return -1;
				}
				FD_CLR(*socket, &readfds);
			}

		}

	} while (len > 0);

	return 1;
}


int Select(SOCKET socket, int recvSend) { // 1 send, 0 recv
	int iResult = 0;


	FD_SET set;
	timeval timeVal;

	while (iResult == 0) {


		FD_ZERO(&set);
		// Add socket we will wait to read from
		FD_SET(socket, &set);
		// Set timeouts to zero since we want select to return
		// instantaneously
		timeVal.tv_sec = 0;
		timeVal.tv_usec = 0;

		if (recvSend == 1) {
			iResult = select(0 /* ignored */, NULL, &set, NULL, &timeVal);		}		else if (recvSend == 0)
		{
			iResult = select(0 /* ignored */, &set, NULL, NULL, &timeVal);
		}
		//dodatti provere za error
		if (iResult == 0) {
			Sleep(50);
			continue;
		}
		else if (iResult == SOCKET_ERROR) {
			printf("Desila se greska prilikom poziva funkcije!\n");
			break;
		}

	}


	return iResult;
}
