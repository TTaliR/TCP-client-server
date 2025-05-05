/* Don't forget to include "wsock32" in the library list. */
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define MAX_REQUESTS 30

const int TIME_PORT = 27216;

/* the checkForAnError function checks if there was an error and handles it accordingly
   @params
   bytesResult = send/receive function result
   ErrorAt = string stating if it was receive or send function
   socket = the socket to close in case there was an error. */
bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket) {
    if (SOCKET_ERROR == bytesResult) {
        printf("Client: Error at %s(): ", ErrorAt);
        printf("%d\n", WSAGetLastError());
        closesocket(socket);
        WSACleanup();
        return true;
    }
    return false;
}

/* function to establish a new connection to the server */
SOCKET establishConnection() {
    SOCKET connSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == connSocket) {
        printf("Client: Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }

    /* initialize server address structure */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(TIME_PORT);

    /* connect to the server */
    if (SOCKET_ERROR == connect(connSocket, (SOCKADDR*)&server, sizeof(server))) {
        printf("Client: Error at connect(): %d\n", WSAGetLastError());
        closesocket(connSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }
    printf("\nConnection established successfully.\n");
    return connSocket;
}


void main() {
    WSADATA wsaData;

    /* Initialize Winsock (Windows Sockets).*/
    if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("Client: Error at WSAStartup()\n");
        return;
    }

    SOCKET connSocket = INVALID_SOCKET; /* status flag that tells us if a connection exists */
    int bytesSent = 0; /* num of bytes sent */
    int bytesRecv = 0; /* num of bytes received */
    char sendBuff[512]; /* buffer for sending data */
    char recvBuff[1024];/* buffer for receiving data */
    char option; /* option the user selected */
    LARGE_INTEGER frequency;  /* ticks per second */
    LARGE_INTEGER start, end;  /* start and end times */
    double interval;  /* RTT in seconds */
    double rtt[MAX_REQUESTS]; /* array to store all RTT */
    int requests[MAX_REQUESTS]; /* array to store all request numbers */
    int request_num = 1;  /* initialize counter for request numbers */
    int total_requests = 0; /* counter to keep track of num of requests made */


    /* ask user to choose connection type */
    printf("\nChoose type of connection:\n");
    printf("1. Non-Persistent\n");
    printf("2. Persistent\n");
    printf("Your selection: ");
    int selection; /* user's selection */

    /* show error if the user didn't enter 1 or 2 */
    while (scanf("%d", &selection) != 1 || (selection != 1 && selection != 2)) {
        printf("Error: Invalid input. Please enter 1 or 2.\n");
        fflush(stdin);
        printf("Your selection: ");
    }
    fflush(stdin);

    /* establish a connection now if the user selected persistent connection */
    if (selection == 2) {
        connSocket = establishConnection();
        if (connSocket == INVALID_SOCKET) {
            WSACleanup();
            return;
        }
    }


    /* main loop for handling user options */
    while (option != 3) {
        /* for non-persistent connection, establish new connection before each request */
        if (selection == 1 && connSocket == INVALID_SOCKET) {
            connSocket = establishConnection();
            if (connSocket == INVALID_SOCKET) {
                WSACleanup();
                return;
            }
        }

        printf("\nPlease insert an option:\n");
        printf("1 : Get file.\n");
        printf("2 : Get file N times.\n");
        printf("3 : Get RTT time measurements.\n");
        printf("4 : Exit.\n");
        printf("Your option: ");
        scanf(" %c", &option);
        fflush(stdin);

        switch (option) {
            case '1': {
                /* single file request */
                printf("Enter filename: ");
                char filename[255];
                scanf("%s", filename);
                fflush(stdin);

                /* format sendBuff */
                strcpy(sendBuff, "REQUEST_FILE: ");
                strcat(sendBuff, filename);

                /* start measuring RTT*/
                QueryPerformanceFrequency(&frequency); /* convert the ticks into seconds */
                QueryPerformanceCounter(&start); /* the time when sending the request */

                /* send the request */
                printf("\nSEND REQUEST #%d: %s\n", request_num, sendBuff);
                bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
                if (checkForAnError(bytesSent, "send", connSocket)) {
                    return;
                }

                /* receive the file content */
                memset(recvBuff, 0, sizeof(recvBuff)); /* clear buffer */
                bytesRecv = recv(connSocket, recvBuff, sizeof(recvBuff) - 1, 0);
                if (checkForAnError(bytesRecv, "recv", connSocket)) {
                    return;
                }
                recvBuff[bytesRecv] = '\0';
                printf("RECEIVE FILE CONTENT: %s", recvBuff);
                QueryPerformanceCounter(&end); /* the time when receiving the response */

                /* calculate and save RTT */
                interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart; /* RTT in seconds */
                printf("\nRTT for this request: %f\n", interval);
                rtt[total_requests] = interval;
                requests[total_requests] = request_num;
                total_requests++;

                /* close the connection if non persistent */
                if (selection == 1) {
                    printf("\nClosing connection.\n");
                    closesocket(connSocket);
                    connSocket = INVALID_SOCKET;  /* mark socket as invalid for next request */
                }
                request_num++;  /* increment request_num after each request is sent */

                break;
            }

            case '2': {
                /* multiple file requests */
                printf("Enter filename: ");
                char filename[255];
                scanf("%s", filename);
                fflush(stdin);

                printf("Times to get the file: ");
                int n, i;
                while (1) {
                    if (scanf("%d", &n) != 1) {  /* check if the input is invalid */
                        printf("Invalid input. Please enter a valid integer for the number of times to get the file.\n");
                        fflush(stdin);
                        printf("Times to get the file: ");
                    }
                    else {
                        break;
                    }
                }

                fflush(stdin);

                /* format sendBuff */
                sprintf(sendBuff, "REQUEST_FILE_N: %d %s", n, filename);

                /* send the request N times, receive N answers */
                for (i = 0; i < n; i++) {
                    printf("\nSEND REQUEST #%d: %s\n", request_num, sendBuff);

                    /* measure RTT for each request */
                    QueryPerformanceFrequency(&frequency); /* convert the ticks into seconds */
                    QueryPerformanceCounter(&start); /* the time when requesting */

                    /* send the request */
                    bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
                    if (checkForAnError(bytesSent, "send", connSocket)) {
                        return;
                    }

                    /* receive the file content */
                    memset(recvBuff, 0, sizeof(recvBuff)); /* clear buffer */
                    bytesRecv = recv(connSocket, recvBuff, sizeof(recvBuff) - 1, 0);
                    if (checkForAnError(bytesRecv, "recv", connSocket)) {
                        return;
                    }
                    recvBuff[bytesRecv] = '\0';  /* null-terminate the received string */
                    printf("\nRECEIVE FILE CONTENT: %s", recvBuff);
                    QueryPerformanceCounter(&end); /* the time when receiving the response */

                    /* calculate and save the RTT */
                    interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart; /* RTT in seconds */
                    printf("\nRTT for this request: %f\n", interval);
                    rtt[total_requests] = interval; /* store RTT for this request */
                    requests[total_requests] = request_num; /* store request num associated with this RTT */
                    total_requests++; /* increment total requests counter to keep track of requests made in total */


                    request_num++;


                    /* for non-persistent mode, close and reopen the connection after each request */
                    if (selection == 1) {
                        printf("\nClosing connection.\n");
                        closesocket(connSocket);

                        /* if this was the last request in non-persistent mode, break from the loop */
                        /* this is to avoid unnecessary re-establishment of the connection */
                        if (i == n - 1) {
                            break;
                        }

                        /* re-establish connection for each incoming file request */
                        connSocket = establishConnection();
                        if (connSocket == INVALID_SOCKET) {
                            printf("Error re-establishing connection.\n");
                            WSACleanup();
                            return;
                        }

                    }
                }

                /* close the connection and mark socket as invalid for the next request */
                if (selection == 1) {
                    closesocket(connSocket);
                    connSocket = INVALID_SOCKET;
                }
                break;
            }

           case '3': {
               /* RTT measurements */
                if (total_requests == 0) {
                    printf("\nNo requests available yet. Please make some requests first.\n");
                    break;
                }
                int i;

                for(i=0;i<total_requests;i++) {
                    printf("\nRequest #%d: %f seconds\n", requests[i], rtt[i]);
                }

                /* calculating avg RTT */
                double sum = 0.0;
                for(i=0;i<total_requests;i++)
                    sum += rtt[i];
                double rttAvg = sum / total_requests;

                printf("\nAverage RTT: %f\n", rttAvg);

                break;
            }

            case '4':
                /* exit */
                if (connSocket != INVALID_SOCKET) {
                    printf("\nClient exiting. Closing the connection.\n");
                    closesocket(connSocket);
                }
                WSACleanup();
                return;

            default: {
                /*invalid option handling */
                strcpy(sendBuff, "INVALID_REQUEST");
                printf("\nSEND REQUEST #%d: %s\n", request_num, sendBuff);
                bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
                if (checkForAnError(bytesSent, "send", connSocket)) {
                    return;
                }

                /* receive error message from server */
                memset(recvBuff, 0, sizeof(recvBuff));
                bytesRecv = recv(connSocket, recvBuff, sizeof(recvBuff) - 1, 0);
                if (checkForAnError(bytesRecv, "recv", connSocket)) {
                    return;
                }
                recvBuff[bytesRecv] = '\0';
                printf("RECEIVE: %s\n", recvBuff);

                /* close the connection if non persistent */
                if (selection == 1) {
                    printf("\nClosing connection.\n");
                    closesocket(connSocket);
                    connSocket = INVALID_SOCKET;  /* mark socket as invalid for next request */
                }

                request_num++;

                break;
            }
        }
    }

}
