#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

typedef struct m
{
	char *content;
	int contentLength;
	struct m *next;
}Message;

typedef struct q
{
	char *name;
	Message *message;
	struct q *next;
}Queue;

bool InitializeWindowsSockets();
int RecievePacket(SOCKET *socket, char * recvBuffer, int length);
int ReciveMessage(SOCKET *socket, char * recvBuffer, int length);
int SendPacket(SOCKET *socket, char * message, int messageSize);
void AddQueue(Queue **head, char* name, int nameSize);
void DodajPorukuNaKraj(Message **listOfMsg, char* msg);
void Ispis(Queue *head);
void IspisPoruka(Queue *head);
bool CheckIfQueueExists(Queue *head, char* queueName);
bool AddMessage(Queue *head, char *queueName, char *message);

int  main(void) 
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
    // Socket used for communication with client
    SOCKET acceptedSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    char recvbuf[DEFAULT_BUFLEN];


    
    if(InitializeWindowsSockets() == false)
    {
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
    }

	int ch = 0;
	Queue *listOfQueues = (Queue*)malloc(sizeof(Queue));
	listOfQueues = NULL;
	//Initialize(listOfQueues);
	char* name = (char*)malloc(sizeof(50));
	memset(name, 0, 1);
	int nameSize = 0;

	while (ch != 4)
	{
		printf("\nServer operations\n");
		printf("1.Add queue\n2.Delete queue\n3.Display queue\n4.Exit\n");
		printf("Enter your choice:");
		scanf("%d", &ch);
		switch (ch)
		{
		case 1:
			printf("Queue you want to add: ");
			fflush(stdin);
			getchar();
			fgets(name, 50, stdin);

			nameSize = strlen(name);
			AddQueue(&listOfQueues, name,nameSize);
			break;
		case 2:
			//printf("Unesite element koji zelite da obrisete: ");
			//scanf("%d\n", val);
			printf("Queue has been deleted\n");

			break;
		case 3:
			printf("Queue display\n");
			Ispis(listOfQueues);
			break;
		case 4: break;
		default:printf("Invalid option\n");
		}

	}

    
    // Prepare address information structures
    addrinfo *resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if ( iResult != 0 )
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
                          SOCK_STREAM,  // stream socket
                          IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind( listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Since we don't need resultingAddress any more, free it
    freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

	printf("Server initialized, waiting for clients.\n");

    do
    {
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.
        acceptedSocket = accept(listenSocket, NULL, NULL);

        if (acceptedSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

		unsigned long mode = 1;
		iResult = ioctlsocket(acceptedSocket, FIONBIO, &mode);

		if (iResult != NO_ERROR)
		{
			printf("ioctlsocket failed with error: %ld\n", iResult);
		}

        do
        {
			int len;
			iResult = RecievePacket(&acceptedSocket, (char*)&len, 4);

			if (iResult != 1)
			{
				if (iResult == -1)
				{
					printf("Recived more bytes than expected.\n");
				}
				else
				{
					printf("Error while getting packets. Error code: %d\n", iResult);
					closesocket(acceptedSocket);
					WSACleanup();
					return 1;
				}
			}

			printf("Length: %d\n", len);
			char *recvBuffer = (char*)malloc(len + 1);
			memset(recvBuffer, 0, 1);

			iResult = ReciveMessage(&acceptedSocket, recvBuffer, len);

			if (iResult != 1)
			{
				if (iResult == -1)
				{
					printf("Recived more bytes than expected.\n");
				}
				else
				{
					printf("Error while getting packets. Error code: %d\n", iResult);
					closesocket(acceptedSocket);
					WSACleanup();
					return 1;
				}
			}

			recvBuffer[len] = 0;

			char *qName = (char*)malloc(100);
			memset(qName, 0, 1);

			char *msg = (char*)malloc(100);
			memset(msg, 0, 1);

			bool delimiter = false;

			int index = 0;


			for (int i = 0; i < strlen(recvBuffer)-1; i++)
			{
				if (recvBuffer[i] != '|')
				{
					qName[i] = recvBuffer[i];
				}
				else
				{
					index = i;
					break;
				}
			}

			qName[index] = 0;

			int j = 0;
			for (int i = index+1; i < strlen(recvBuffer) - 1; i++)
			{
				msg[j] = recvBuffer[i];
				j++;
			}

			msg[j] = 0;
			

			if (CheckIfQueueExists(listOfQueues, qName))
			{
				AddMessage(listOfQueues, qName, msg);

				char* message = (char*)malloc(100);
				message = "Message has been added to queue!jebem ti mater\n";
				//printf("Write message to send: ");
				//fflush(stdin);
				//fgets(message, 100, stdin);

				int messageSize = strlen(message);

				iResult = SendPacket(&acceptedSocket, (char*)&messageSize, sizeof(int));
				iResult = SendPacket(&acceptedSocket, message, messageSize);

				Ispis(listOfQueues);
			}
			else
			{
				char* message = (char*)malloc(100);
				message = "Queue you requested doesn't exist!jebem ti mater\n";
				//printf("Write message to send: ");
				//fflush(stdin);
				//fgets(message, 100, stdin);

				int messageSize = strlen(message);

				iResult = SendPacket(&acceptedSocket, (char*)&messageSize, sizeof(int));
				iResult = SendPacket(&acceptedSocket, message, messageSize);
				/*printf("Ne postoji\n");*/
			}



			//printf("Message recived: %s\n", recvBuffer);
			free(recvBuffer);

			Sleep(1000);


			//************************************************************************************************************************

			//char* message = (char*)malloc(100);
			////printf("Write message to send: ");
			//fflush(stdin);
			//fgets(message, 100, stdin);

			//int messageSize = strlen(message);

			//iResult = SendPacket(&acceptedSocket, (char*)&messageSize, sizeof(int));
			//iResult = SendPacket(&acceptedSocket, message, messageSize);

			/*Sleep(3000);*/


            //// Receive data until the client shuts down the connection
            //iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            //if (iResult > 0)
            //{
            //    printf("Message received from client: %s.\n", recvbuf);
            //}
            //else if (iResult == 0)
            //{
            //    // connection was closed gracefully
            //    printf("Connection with client closed.\n");
            //    closesocket(acceptedSocket);
            //}
            //else
            //{
            //    // there was an error during recv
            //    printf("recv failed with error: %d\n", WSAGetLastError());
            //    closesocket(acceptedSocket);
            //}

		} while (1);//(iResult > 0);

        // here is where server shutdown loguc could be placed

    } while (1);

    // shutdown the connection since we're done
    iResult = shutdown(acceptedSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(listenSocket);
    closesocket(acceptedSocket);
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



void AddQueue(Queue **head, char* name, int nameSize) 
{
	Queue *newQ = (Queue*)malloc(sizeof(Queue));
	newQ->name = (char*)malloc(nameSize);

	for (int i = 0; i < nameSize-1; i++)
	{
		newQ->name[i] = name[i];
	}

	newQ->name[nameSize-1] = 0;

	//*(newQ->name) = *(int*)name;
	newQ->next = NULL;
	newQ->message = NULL;

	if (*head == NULL)
	{
		*head = newQ;
		//q = *head;
	}
	else
	{

		Queue *pomocna = *head;
		while (pomocna->next != NULL)
		{
			pomocna = pomocna->next;
		}
		pomocna->next = newQ;
		//q = *head;
	}

	printf("List of queues:\n");
	Ispis(*head);
}

void Ispis(Queue *head)
{
	printf("\n");
	printf("___________________\n");
	printf("Queue: %s\n", head->name);
	Message *m = (Message*)malloc(sizeof(Message));
	m = head->message;

	if (m != NULL)
		IspisPoruka(head);

	if (head->next == NULL)
	{
		printf("\n");
		printf("___________________\n");
		return;
	}

	printf("___________________\n");
	Ispis(head->next);
}

void IspisPoruka(Queue *head)
{
	Message *m = (Message*)malloc(sizeof(Message));
	Queue *q = (Queue*)malloc(sizeof(Queue));
	q = head;
	m = head->message;
	printf("\nMessage from queue %s:\n", q->name);


	while (m != NULL)
	{
		printf("Message: %s\n", m->content);
		printf("Length: %d\n", m->contentLength);
		m = m->next;
	}
}

int ReciveMessage(SOCKET *socket, char * recvBuffer, int length)
{
	int primio = 0;
	int len = length;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(*socket, &readfds);

	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	do
	{
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
					printf("Message %s\n", recvBuffer);
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



int RecievePacket(SOCKET *socket, char * recvBuffer, int length)
{
	int primio = 0;
	int len = length;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(*socket, &readfds);

	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	do
	{
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

int SendPacket(SOCKET *socket, char * message, int messageSize)
{
	int poslao = 0;
	int msgSize = messageSize;
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(*socket, &writefds);
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	do
	{
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

bool CheckIfQueueExists(Queue *head, char* queueName)
{
	Queue *pom = (Queue*)malloc(sizeof(Queue));
	bool found = false;
	pom = head;

	do
	{
		if (pom->name != NULL)
		{
			if (strcmp(pom->name,queueName)==0)
			{
				found = true;
				return true;
			}
			else
			{
				if (pom->next == NULL)
					break;
				else
					pom = pom->next;
			}
		}
	} while (!found);
	return false;
}

bool AddMessage(Queue *head, char *queueName, char *message)
{
	Message *msg = (Message*)malloc(sizeof(Message));

	Queue *pom = (Queue*)malloc(sizeof(Queue));
	Message *PomMsg = (Message*)malloc(sizeof(Message));
	bool found = false;
	pom = head;

	do
	{
		if (pom->name != NULL)
		{
			if (strcmp(pom->name, queueName) == 0)
			{
				found = true;
				DodajPorukuNaKraj(&(pom->message), message);
			}
			else
			{
				if (pom->next == NULL)
					break;
				else
					pom = pom->next;
			}
		}
	} while (!found);
	return false;
}


void DodajPorukuNaKraj(Message **listOfMsg, char* msg)
{
	Message *novi = (Message*)malloc(sizeof(Message));
	novi->content = msg;
	novi->contentLength = strlen(msg);
	novi->next = NULL;
	if (*listOfMsg == NULL)
	{
		*listOfMsg = novi;
	}
	else
	{
		Message *pom = *listOfMsg;
		while (pom->next != NULL)
		{
			pom = pom->next;
		}
		pom->next = novi;
	}
}