/* Don't forget to include "wsock32" in the library list. */
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

const int TIME_PORT = 27216;

/* function to read the content of a file from the "Files" folder. */
char *readFile(char *filename) {
    char fullPath[512];
    sprintf(fullPath, "Files/%s", filename);  /* read from the "Files" folder */
    FILE *f = fopen(fullPath, "rt");
    if (!f) {  /* file doesn't exist or can't be opened */
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *)malloc(length + 1);
    memset(buffer, 0, length + 1);
    buffer[length] = '\0';
    fread(buffer, 1, length, f);
    fclose(f);
    return buffer;
}

/* The checkForAnError function checks if there was an error and handles it accordingly
// @ params
// bytesResult = send/receive function result
// ErrorAt = string stating if it was receive or send function
// socket_1 & socket_2 = the sockets to close in case there was an error. */

bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket_1, SOCKET socket_2) {
    if (SOCKET_ERROR == bytesResult) {
        printf("Server: Error at %s(): ", ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket_1);
        closesocket(socket_2);
        WSACleanup();
        return true;
    }
    return false;
}

void main() {
    /* Initialize Winsock (Windows Sockets).
    // Create a WSADATA object called wsaData.
    // The WSADATA structure contains information about the Windows
    // Sockets implementation. */
    WSADATA wsaData;
    SOCKET listenSocket;
    struct sockaddr_in serverService;
    int request_num = 0; /* initialize counter for requests */

    /* Call WSAStartup and return its value as an integer and check for errors.
    // The WSAStartup function initiates the use of WS2_32.DLL by a process.
    // First parameter is the version number 2.2.
    // The WSACleanup function destructs the use of WS2_32.DLL by a process. */
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("Server: Error at WSAStartup()\n");
        return;
    }

    /* Create the listen socket */
    listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == listenSocket) {
        printf("Server: Error at socket(): ");
        printf("%d", WSAGetLastError());
        WSACleanup();
        return;
    }

    memset(&serverService, 0, sizeof(serverService));
    /* Address family (must be AF_INET - Internet address family). */
    serverService.sin_family = AF_INET;
    /* IP address. The sin_addr is a union (s_addr is an unsigned long
    // (4 bytes) data type).
    // inet_addr (Internet address) is used to convert a string (char *)
    // into unsigned long.
    // The IP address is INADDR_ANY to accept connections on all interfaces. */
    serverService.sin_addr.s_addr = htonl(INADDR_ANY);
    /* IP Port. The htons (host to network - short) function converts an
    // unsigned short from host to TCP/IP network byte order
    // (which is big-endian). */
    serverService.sin_port = htons(TIME_PORT);

    /* Bind the socket for client's requests.
    // The bind function establishes a connection to a specified socket.
    // The function uses the socket handler, the sockaddr structure (which
    // defines properties of the desired connection) and the length of the
    // sockaddr structure (in bytes). */
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService))) {
        printf("Server: Error at bind(): ");
        printf("%d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    /* Listen on the Socket for incoming connections.
    // This socket accepts only one connection (no pending connections
    // from other clients). This sets the backlog parameter.
    // defines the maximum length to which the
    // queue of pending connections for listenSocket may grow */
    if (SOCKET_ERROR == listen(listenSocket, 5)) {
        printf("Server: Error at listen(): ");
        printf("%d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    while (1) {
        /* Accept connections and handles them one by one. */
        struct sockaddr_in from; /* Address of sending partner */
        int fromLen = sizeof(from);

        printf("\nServer: Wait for clients' requests.\n");

        /* The accept function permits an incoming connection
        // attempt on another socket (msgSocket).
        // The first argument is a bounded-listening socket that
        // receives connections.
        // The second argument is an optional pointer to buffer
        // that receives the internet address of the connecting entity.
        // the third one is a pointer to the length of the network address
        // (given in the second argument). */
        SOCKET msgSocket = accept(listenSocket, (struct sockaddr *)&from, &fromLen);
        if (INVALID_SOCKET == msgSocket) {
            printf("Server: Error at accept(): ");
            printf("%d", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        printf("Server: Client is connected.\n");

        while (1) {

            /* Send and receive data. */
            int bytesSent = 0;
            int bytesRecv = 0;
            char *sendBuff; /* buffer for sending data */
            char recvBuff[4096]; /* buffer for receiving data */
            LARGE_INTEGER frequency; /* ticks per second */
            LARGE_INTEGER start, end; /* timestamps for the start and end of the request */
            double interval; /* time to process each request in seconds */


            /* Get client's requests and answer them.
            // The recv function receives data from a connected or bound socket.
            // The buffer for data to be received and its available size are
            // returned by recv. The last argument is an indicator specifying the way
            // in which the call is made (0 for default). */

            /* receive data from the client */
            bytesRecv = recv(msgSocket, recvBuff, 255, 0);
            if (checkForAnError(bytesRecv, "recv", listenSocket, msgSocket))
                return;

            /* if client closes the connection, close the server message socket too to release resources associated with the client */
            if (bytesRecv <= 0) {
                printf("\nClosing connection.\n");
                closesocket(msgSocket);
                break;
            }

            request_num++; /* increment counter for each request made by the client */
            recvBuff[bytesRecv] = '\0'; /* clear buffer */
            printf("\nRECEIVE REQUEST #%d: %s\n", request_num, recvBuff);

            /* start processing the client request */

            QueryPerformanceFrequency(&frequency);

            QueryPerformanceCounter(&start); /* start time when request is received */

            if (strncmp(recvBuff, "REQUEST_FILE: ", 14) == 0) { /* check if the recvBuff starts with the file request */
                char filename[255];
                strcpy(filename, recvBuff + 14);  /* get file name, skip 14 first chars */

                char *fileContent = readFile(filename);  /* read the file */
                if (fileContent == NULL) {
                    sendBuff = "Error: File not found or could not be read.\n";
                    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
                    printf("SEND: %s", sendBuff);

                } else {
                    /* separate buffer for the file content */
                    sendBuff = (char *)malloc(strlen(fileContent) + 1);
                    strcpy(sendBuff, fileContent); /* copy file content into sendBuff */

                    /* send the message to the client */
                    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
                    if (checkForAnError(bytesSent, "send", listenSocket, msgSocket)) {
                        free(fileContent);
                        return;
                    }

                    QueryPerformanceCounter(&end); /* end time when the request is finished processing */

                    interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart; /* the time to process the request in seconds */

                    printf("SEND: %s", sendBuff);
                    printf("\nPROCESS TIME: %f\n", interval);
                    free(fileContent);  /* free the buffer */
                }
            }
            else if (strncmp(recvBuff, "REQUEST_FILE_N: ", 16) == 0) {
                QueryPerformanceFrequency(&frequency);

                QueryPerformanceCounter(&start); /* start time when request is received */

                char filename[255];
                int n, i;

                /* get the filename and number of times to send the file */
                sscanf(recvBuff + 16, "%d %s", &n, filename);

                /* read the file content */
                char *fileContent = readFile(filename);
                if (fileContent == NULL) {
                    sendBuff = "Error: File not found or could not be read.\n";
                    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
                    printf("SEND: %s", sendBuff);
                }
                else {
                    /* format the message to be sent */
                    sendBuff = (char *)malloc(strlen(fileContent) + 1);
                    strcpy(sendBuff, fileContent); /* copy file content into sendBuff */

                    /* send the message to the client */
                    bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
                    if (checkForAnError(bytesSent, "send", listenSocket, msgSocket)) {
                        free(fileContent);
                        return;
                    }
                    QueryPerformanceCounter(&end); /* end time when the request is finished processing */

                    interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart; /* the time to process the request in seconds */

                    printf("SEND: %s", sendBuff);
                    printf("\nPROCESS TIME: %f\n", interval);


                    free(fileContent);
                }
            }



            /* in case the request is invalid */
            else if (strcmp(recvBuff, "INVALID_REQUEST") == 0) {
                QueryPerformanceFrequency(&frequency);

                QueryPerformanceCounter(&start); /* start time when request is received */

                sendBuff = "Error: Invalid menu option. Please choose options 1, 2, 3 or 4 only.";
                bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
                if (checkForAnError(bytesSent, "send", listenSocket, msgSocket)) {
                    return;
                }
                QueryPerformanceCounter(&end); /* end time when the request is finished processing */

                interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart; /* the time to process the request in seconds */


                printf("SEND: %s\n", sendBuff);
                printf("PROCESS TIME: %f\n", interval);

            }

        }
    }

    closesocket(listenSocket);
    WSACleanup();
}
