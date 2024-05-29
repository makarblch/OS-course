#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include "diewitherror.cpp"

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define ARRSIZE 100 /* Size of array */

int database[ARRSIZE];
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logsem = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
int active_readers = 0;
bool writer_active = false;
std::string logger = "0";

void FillArray() {
    for (int i = 0; i < 100; ++i) {
        database[i] = i;
    }
}

void SafeLog(const char *message) {
    pthread_mutex_lock(&logsem);
    if (logger == "0") {
        logger = "";
    }
    logger += std::string(message);
    pthread_mutex_unlock(&logsem);
}

void SendMessageToClient(int clntSocket, const std::string &message) {
    if (send(clntSocket, message.c_str(), message.length(), 0) != message.length()) {
        DieWithError("send() failed");
    }
}

void ChangeValue(int current_client_socket, int index, int number) {
    database[index] = number;
    printf("[SERVER] Writer successfully changed value %d at index %d!\n", number, index);
    char buf[256];
    sprintf(buf, "[SERVER] Writer successfully changed value %d at index %d!\n", number, index);
    SafeLog(buf);
    SendMessageToClient(current_client_socket, "Changed!");
    std::sort(std::begin(database), std::end(database));
}

void HandleReader(int current_client_socket, const std::string &string) {
    pthread_mutex_lock(&mtx);
    while (writer_active) {
        pthread_cond_wait(&cv, &mtx);
    }
    ++active_readers;
    pthread_mutex_unlock(&mtx);

    try {
        int index = std::stoi(string.substr(1));
        if (index >= 0 && index < ARRSIZE) {
            std::string message = std::to_string(database[index]);
            printf("[SERVER] Reader %d is reading at index %d. Value = %d.\n", current_client_socket,
                   index, database[index]);
            char buf[100];
            sprintf(buf, "[SERVER] Reader %d is reading at index %d. Value = %d.\n", current_client_socket,
                    index, database[index]);
            SafeLog(buf);
            SendMessageToClient(current_client_socket, message);
        } else {
            SendMessageToClient(current_client_socket, "[SERVER->READER] Index out of range\n");
            SafeLog("[SERVER->READER] Index out of range\n");
        }
    } catch (const std::invalid_argument &e) {
        SendMessageToClient(current_client_socket, "[SERVER->READER] Invalid index\n");
        SafeLog("[SERVER->READER] Invalid index\n");
    }

    pthread_mutex_lock(&mtx);
    --active_readers;
    if (active_readers == 0) {
        pthread_cond_broadcast(&cv);
    }
    pthread_mutex_unlock(&mtx);
}

void HandleWriter(int current_client_socket, const std::string &string) {
    std::size_t firstSpace = string.find(' ');
    if (firstSpace == std::string::npos) {
        SendMessageToClient(current_client_socket, "[SERVER->WRITER] Invalid input\n");
        SafeLog("[SERVER->WRITER] Invalid input\n");
        return;
    }

    std::string indexStr = string.substr(1, firstSpace - 1);
    std::string numberStr = string.substr(firstSpace + 1);

    int index = 0;
    int number = 0;

    try {
        index = std::stoi(indexStr);
        number = std::stoi(numberStr);
    } catch (const std::exception &e) {
        SendMessageToClient(current_client_socket, "[SERVER->WRITER] Invalid arguments\n");
        SafeLog("[SERVER->WRITER] Invalid input\n");
        return;
    }

    if (index >= 0 && index < ARRSIZE && number < 1000) {
        pthread_mutex_lock(&mtx);
        while (writer_active || active_readers > 0) {
            pthread_cond_wait(&cv, &mtx);
        }
        writer_active = true;
        pthread_mutex_unlock(&mtx);

        printf("[SERVER] Writer %d is rewriting at index %d. New value = %d.\n", current_client_socket,
               index, number);
        char buf[256];
        sprintf(buf, "[SERVER] Writer %d is rewriting at index %d. New value = %d.\n", current_client_socket,
                index, number);
        SafeLog(buf);
        ChangeValue(current_client_socket, index, number);
        pthread_mutex_lock(&mtx);
        writer_active = false;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&mtx);
    } else {
        SendMessageToClient(current_client_socket, "[SERVER->WRITER] Index out of range\n");
        SafeLog("[SERVER->WRITER] Index out of range\n");
    }
}

char *HandleTCPClient(int clntSocket) {
    char *buffer = new char[RCVBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;                            /* Size of received message */

    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    buffer[recvMsgSize] = '\0'; // Null terminate the string
    return buffer;
}

void *ManageServerRequest(void *servSockPtr) {
    int servSock = *(int *) servSockPtr;
    struct sockaddr_in client_address{};
    int current_client_socket;
    int client_len = sizeof(client_address);
    unsigned int clntLen = sizeof(client_address);
    if ((current_client_socket = accept(servSock, (struct sockaddr *) &client_address,
                                        &clntLen)) < 0) {
        DieWithError("Problem with accept()");
    }
    printf("[SERVER] Make connection with client %d\n", current_client_socket);
    char buf[256];
    sprintf(buf, "[SERVER] Make connection with client %d\n", current_client_socket);
    SafeLog(buf);

    while (true) {
        const char *buffer = HandleTCPClient(current_client_socket);
        std::string string(buffer);

        if (string.substr(1, 1) == "-") {
            printf("[SERVER] End of connection with client %d.\n", current_client_socket);
            char buf[256];
            sprintf(buf, "[SERVER] End of connection with client %d.\n", current_client_socket);
            SafeLog(buf);
            close(current_client_socket);
            delete[] buffer;
            break;
        } else if (buffer[0] == 'r') {
            HandleReader(current_client_socket, string);
        } else if (buffer[0] == 'w') {
            HandleWriter(current_client_socket, string);
        } else if (buffer[0] == 'l') {
            SendMessageToClient(current_client_socket, logger);
            logger = "0";
        }
        delete[] buffer;
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    int servSock;                    /* Socket descriptor for server */
    int readers;
    int writers;
    int N;
    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr{}; /* Local address */
    struct sockaddr_in echoClntAddr{}; /* Client address */
    unsigned short echoServPort;     /* Server port */
    FillArray();

    if (argc != 4)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Readers> <Writers>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    readers = atoi(argv[2]);       /* Second arg: number of readers */
    writers = atoi(argv[3]);       /* Third arg: number of writers */
    N = writers + readers + 1;

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    if (listen(servSock, 5) < 0)  /* Listen for incoming connections */
        DieWithError("listen() failed");

    printf("Server is running on port %d. Waiting for connections...\n", echoServPort);
    char buf[256];
    sprintf(buf, "Server is running on port %d. Waiting for connections...\n", echoServPort);
    SafeLog(buf);
    pthread_t threads[N];

    for (int i = 0; i < N; ++i) {
        if (pthread_create(&threads[i], nullptr, ManageServerRequest, &servSock) != 0) {
            DieWithError("pthread_create() failed");
        }
    }

    /* Wait until all threads finish working */
    for (int i = 0; N; ++i) {
        pthread_join(threads[i], nullptr);
    }

    close(servSock);
    return 0;
}
