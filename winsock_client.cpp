#define WIN32_LEAN_AND_MEAN
#define _WINNT_WIN32 0x0601

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

// Struct for the queue
struct printJob {
    char clientName[50];
    char content[100];
};

void menu() {
    printf("\nMenu de opcoes:\n");
    printf("1 - Adicionar elemento na lista de impressao\n");
    printf("2 - Listar elementos da lista de impressao\n");
    printf("3 - Sair\n");
}

int __cdecl main(int argc, char **argv) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char sendbuf[DEFAULT_BUFLEN];  
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    memset(sendbuf, '\0', sizeof(sendbuf));
    struct printJob job;

    int option;
    bool hasElements = false;
    do {
        menu();
        printf("\nDigite a opcao desejada: ");
        scanf("%d", &option);
        fflush(stdin);

        switch (option) {
            case 1:
            	size_t len;
            	size_t lenC;
            	
                // Requests customer name
                printf("\nDigite o nome do cliente: ");
                if (fgets(job.clientName, DEFAULT_BUFLEN, stdin) == NULL) {
                    perror("fgets");
                    return 1;
                }
                // Remove the newline character (if any)
                len = strlen(job.clientName);
                if (len > 0 && job.clientName[len - 1] == '\n') {
                    job.clientName[len - 1] = '\0';
                }
                // Check if the name exceeds 50 characters
                if (len > 50) {
                    printf("Por favor, insira um nome de ate 50 caracteres.\n");
                    printf("Aperte qualquer tecla para digitar novamente.\n");
					getch();
                }
                // Requests content to be printed
                printf("Digite o conteudo a ser impresso: ");
                if (fgets(job.content, DEFAULT_BUFLEN, stdin) == NULL) {
                    perror("fgets");
                    return 1;
                }
                // Remove the newline character (if any)
                lenC = strlen(job.content);
                if (len > 0 && job.content[lenC - 1] == '\n') {
                    job.content[lenC - 1] = '\0';
                }
                // Check if the content exceeds 100 characters
                if (lenC > 100) {
                    printf("Conteúdo muito extenso. Por favor descreva ate 100 caracteres.\n");
                    printf("Aperte qualquer tecla para digitar novamente.\n");
					getch();
                }
                // Concatenate the client name and the content to be sent
                memset(sendbuf, '\0', sizeof(sendbuf));
                sprintf(sendbuf, "%s %s", job.clientName, job.content);
                
                // Validate the parameters
			    if (argc != 2) {
			        printf("usage: %s server-name\n", argv[0]);
			        return 1;
			    }		
			    // Initialize Winsock
			    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			    if (iResult != 0) {
			        printf("WSAStartup failed with error: %d\n", iResult);
			        return 1;
			    }		
			    ZeroMemory( &hints, sizeof(hints) );
			    hints.ai_family = AF_UNSPEC;
			    hints.ai_socktype = SOCK_STREAM;
			    hints.ai_protocol = IPPROTO_TCP;		
			    // Resolve the server address and port
			    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
			    if ( iResult != 0 ) {
			        printf("getaddrinfo failed with error: %d\n", iResult);
			        WSACleanup();
			        return 1;
			    }			
			    // Attempt to connect to an address until one succeeds
			    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {		
			        // Create a SOCKET for connecting to server
			        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
			            ptr->ai_protocol);
			        if (ConnectSocket == INVALID_SOCKET) {
			            printf("socket failed with error: %ld\n", WSAGetLastError());
			            WSACleanup();
			            return 1;
			        }		
			        // Connect to server
			        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			        if (iResult == SOCKET_ERROR) {
			            closesocket(ConnectSocket);
			            ConnectSocket = INVALID_SOCKET;
			            continue;
			        }
		        	break;
		    	}
		    	freeaddrinfo(result);
			    if (ConnectSocket == INVALID_SOCKET) {
			        printf("Unable to connect to server!\n");
			        WSACleanup();
			        return 1;
			    }
			    // Send an initial buffer
			    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
			    if (iResult == SOCKET_ERROR) {
			        printf("send failed with error: %d\n", WSAGetLastError());
			        closesocket(ConnectSocket);
			        WSACleanup();
			        return 1;
			    }		
                printf("Bytes Sent: %ld\n", iResult);
                // Shutdown the connection since no more data will be sent
			    iResult = shutdown(ConnectSocket, SD_SEND);
			    if (iResult == SOCKET_ERROR) {
			        printf("shutdown failed with error: %d\n", WSAGetLastError());
			        closesocket(ConnectSocket);
			        WSACleanup();
			        return 1;
			    }
                hasElements = true;
                break;
            case 2:
            	if (!hasElements) {
                    printf("\nLista vazia, digite 1 para adicionar elementos na lista de impressao. \n");
                    break;
                }
                // Receive until the peer closes the connection
                memset(recvbuf, '\0', sizeof(recvbuf)); // Clear the buffer
                iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
                if (iResult > 0) {
                    printf("Bytes received: %d\n", iResult);
                    // Treatments to print the list
                    printf("\nLISTA DE IMPRESSAO RECEBIDA: \n");
                    printf("%s\n", recvbuf);
                    printf("Cliente\t\tConteudo\t\tData\t\tHora\n");
                    char *token = strtok(recvbuf, "\n"); // Separates the records from the list
                    // Process each record in the list
					while (token != NULL) {
					    char clientName[50], content[100], date[11], time[9];					
					    // Separate the fields
					    sscanf(token, "%49s %99s %10s %8s", clientName, content, date, time);
                        // Print the fields
					    printf("%s\t\t%s\t\t%s\t\t%s\n", clientName, content, date, time); 
                        // Get the next record			
					    token = strtok(NULL, "\n"); 
					}
                } 
                break;
            case 3:
                printf("\nSAINDO...\n");
                closesocket(ConnectSocket);
		   		WSACleanup(); 
		   		exit;
                break;
            default:
                printf("\nOpcao invalida. Por favor, escolha uma opcao do menu.\n");
                break; 
        }
        system("pause");
		system("cls");
    } while (option != 3);
    
    return 0;
}