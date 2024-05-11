#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <time.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

// Struct for the queue
typedef struct node {
    char clientName[50];
    char content[100];
    char date[40];
    node* next;
} node;

typedef struct fila {
    node* front;
    node* rear;
    int size;
} fila;

struct tm *tm_now;

// Function prototypes
fila* createQueue();
void enqueue(fila* q, char* clientName, char* content);
void printQueue(fila* q, int jobCounter);

int __cdecl main(void) {
    // Inicialize the queue
    fila* q = createQueue();
    remove("lista-de-impressao.txt");
    int jobCounter = 0;

    WSADATA wsaData;
    int iResult;
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    // Receive until the peer shuts down the connection
    bool abresocket=true;
    do {       
        if (abresocket){
		    // Initialize Winsock
		    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		    if (iResult != 0) {
		        printf("WSAStartup failed with error: %d\n", iResult);
		        return 1;
		    }
		    ZeroMemory(&hints, sizeof(hints));
		    hints.ai_family = AF_INET;
		    hints.ai_socktype = SOCK_STREAM;
		    hints.ai_protocol = IPPROTO_TCP;
		    hints.ai_flags = AI_PASSIVE;
		    // Resolve the server address and port
		    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		    if ( iResult != 0 ) {
		        printf("getaddrinfo failed with error: %d\n", iResult);
		        WSACleanup();
		        return 1;
		    }
		    // Create a SOCKET for the server to listen for client connections
		    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		    if (ListenSocket == INVALID_SOCKET) {
		        printf("socket failed with error: %ld\n", WSAGetLastError());
		        freeaddrinfo(result);
		        WSACleanup();
		        return 1;
		    }
		    // Setup the TCP listening socket
		    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		    if (iResult == SOCKET_ERROR) {
		        printf("bind failed with error: %d\n", WSAGetLastError());
		        freeaddrinfo(result);
		        closesocket(ListenSocket);
		        WSACleanup();
		        return 1;
		    }
		    freeaddrinfo(result);
		    iResult = listen(ListenSocket, SOMAXCONN);
		    if (iResult == SOCKET_ERROR) {
		        printf("listen failed with error: %d\n", WSAGetLastError());
		        closesocket(ListenSocket);
		        WSACleanup();
		        return 1;
		    }
		    // Accept a client socket
		    ClientSocket = accept(ListenSocket, NULL, NULL);
		    if (ClientSocket == INVALID_SOCKET) {
		        printf("accept failed with error: %d\n", WSAGetLastError());
		        closesocket(ListenSocket);
		        WSACleanup();
		        return 1;
		    }
		    // No longer need server socket
		    closesocket(ListenSocket);
		}
	    abresocket=false;
        memset(recvbuf, '\0', sizeof(recvbuf));
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            // Separate the client name and the content to be printed
            char clientName[50] = {0};
            char content[100] = {0};
            sscanf(recvbuf, "%s %s", &clientName, &content);
            // Print the client name and the content received
            printf("Nome do cliente: %s\n", clientName);
            printf("Conteudo a ser impresso: %s\n", content); 
            // Add the client name and the content to the queue
            enqueue(q, clientName, content);  
            // Concatenate the queue data into a string
			node* n = q->front;
			char tempbuf[DEFAULT_BUFLEN] = {0};  // Temporary buffer for concatenating the queue data
			strcpy(tempbuf, "");  // Inicialize the buffer
            // Concatenate the queue data into a string
			while (n != NULL) {
			    strcat(tempbuf, n->clientName);
			    strcat(tempbuf, " ");
			    strcat(tempbuf, n->content);
			    strcat(tempbuf, " ");
			    strcat(tempbuf, n->date);
			    strcat(tempbuf, "\n");
			    n = n->next;
			}
            // Copy the concatenated queue data to the recvbuf
            strcpy(recvbuf, tempbuf);
            printf("Lista de impressao concatenada: \n");
            printf("%s\n", recvbuf);

            // Echo the buffer back to the sender
            iSendResult = send( ClientSocket, recvbuf, recvbuflen, 0 );
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
            // Shutdown the connection since we're done
		    iResult = shutdown(ClientSocket, SD_SEND);
		    if (iResult == SOCKET_ERROR) {
		        printf("shutdown failed with error: %d\n", WSAGetLastError());
		        closesocket(ClientSocket);
		        WSACleanup();
		        return 1;
		    }

            // Calls document printing every 3 elements added to the list
            if(q->size == 3) {
                printf("Imprimindo...\n");
                jobCounter = jobCounter + 1;
                printQueue(q, jobCounter);
            }
            // Clear the buffers
            memset(clientName, '\0', sizeof(clientName));
            memset(content, '\0', sizeof(content));
            memset(tempbuf, '\0', sizeof(tempbuf));
		    // Cleanup
		    closesocket(ClientSocket);
		    WSACleanup();
		    abresocket=true;
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
        
    } while (true);

    // Shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    // Cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

fila* createQueue() {
    fila* q = (fila*)malloc(sizeof(fila));
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    return q;
}

void enqueue(fila* q, char* clientName, char* content) {
    node* n = (node*)malloc(sizeof(node));
    strcpy(n->clientName, clientName);
    strcpy(n->content, content);
    time_t now = time(NULL);
    tm_now = localtime(&now);
    strftime(n->date, sizeof(n->date), "%Y-%m-%d %H:%M:%S", tm_now);
    n->next = NULL;
    if (q->size == 0) {
        q->front = n;
        q->rear = n;
    } else {
        q->rear->next = n;
        q->rear = n;
    }
    q->size++;
}

void printQueue(fila* q, int jobCounter) {
    node* n = q->front;    
    std::ofstream arq;
    arq.open("lista-de-impressao.txt", std::ios::app);

    arq<<("\t\t----- IMPRESSÃO DE DADOS ");
    arq<<jobCounter;
    arq<<(" -----\n");
    arq<<("\t\tCliente\t\tConteudo\t\tData\n");
    while (n != NULL) {
        arq<<("\t\t");
        arq<<n->clientName;
        arq<<("\t\t");
        arq<<n->content;
        arq<<("\t\t");
        arq<<n->date;
        arq<<("\n");
        n = n->next;
    }
    q->front = n;
    q->rear = NULL;
    arq<<("\t\t-----   FIM ARQUIVO    -----\n");
    arq.close();
    q->size = 0;
    printf ("Arquivo impresso!\n");
    printf ("====================================\n");
}

