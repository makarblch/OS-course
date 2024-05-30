#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include "diewitherror.cpp"
#include <signal.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define ECHOMAX 255
#define ARRSIZE 100 /* Size of array */

int database[ARRSIZE];
struct sockaddr_in echoClntAddr{}; /* Client address */
std::string logger = "0";
std::map<int, int> logs;

void FillArray() {
    for (int i = 0; i < 100; ++i) {
        database[i] = i;
    }
}

void cleanup() {
    logger = "0";
    for (auto &pair : logs) {
        pair.second = 0;
    }
    sleep(10);
}

void SafeLog(const char *message) {
    if (logger == "0") {
        logger = "";
    }
    logger += std::string(message);
}

void SendLogsToClient(int sock, const std::string &message, int index) {
    std::string message1 = message.substr(index);
    if (index == message.length()) {
        message1 = "0";
    }
    if (sendto(sock, message1.c_str(), message1.length(), 0,
               (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != message1.length())
        DieWithError("sendto() sent a different number of bytes than expected");
}

void SendMessageToClient(int sock, const std::string &message) {
    if (sendto(sock, message.c_str(), message.length(), 0,
               (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != message.length())
        DieWithError("sendto() sent a different number of bytes than expected");
}

void ChangeValue(int sock, int index, int number) {
    database[index] = number;
    printf("[SERVER] Writer successfully changed value %d at index %d!\n", number, index);
    char buf[256];
    sprintf(buf, "[SERVER] Writer successfully changed value %d at index %d!\n", number, index);
    SafeLog(buf);
    SendMessageToClient(sock, "Changed!");
    std::sort(std::begin(database), std::end(database));
}

void HandleReader(int sock, const std::string &string) {
    try {
        int index = std::stoi(string.substr(1));
        if (index >= 0 && index < ARRSIZE) {
            std::string message = std::to_string(database[index]);
            printf("[SERVER] Reader %d is reading at index %d. Value = %d.\n", echoClntAddr.sin_port,
                   index, database[index]);
            char buf[100];
            sprintf(buf, "[SERVER] Reader %d is reading at index %d. Value = %d.\n", echoClntAddr.sin_port,
                    index, database[index]);
            SafeLog(buf);
            SendMessageToClient(sock, message);
        } else {
            SendMessageToClient(sock, "[SERVER->READER] Index out of range");
        }
    } catch (const std::invalid_argument &e) {
        SendMessageToClient(sock, "[SERVER->READER] Invalid index");
    }
}

void HandleWriter(int sock, const std::string &string) {
    std::size_t firstSpace = string.find(' ');
    if (firstSpace == std::string::npos) {
        SendMessageToClient(sock, "[SERVER->WRITER] Invalid input");
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
        SendMessageToClient(sock, "[SERVER->WRITER] Invalid arguments");
        return;
    }

    if (index >= 0 && index < ARRSIZE && number < 1000) {
        printf("[SERVER] Writer %d is rewriting at index %d. New value = %d.\n", echoClntAddr.sin_port,
               index, number);

        char buf[256];
        sprintf(buf, "[SERVER] Writer %d is rewriting at index %d. New value = %d.\n", echoClntAddr.sin_port,
                index, number);
        SafeLog(buf);
        ChangeValue(sock, index, number);
    } else {
        SendMessageToClient(sock, "[SERVER->WRITER] Index out of range");
    }
}

void HandleUDPClient(int sock) {
    int cliAddrLen = sizeof(echoClntAddr);

    char *buffer = new char[ECHOMAX];        /* Buffer for string */
    int recvMsgSize;                            /* Size of received message */

    if ((recvMsgSize = recvfrom(sock, buffer, ECHOMAX, 0,
                                (struct sockaddr *) &echoClntAddr, (socklen_t *) &cliAddrLen)) < 0)
        DieWithError("recvfrom() failed");

    if (logs.find(echoClntAddr.sin_port) == logs.end()) {
        printf("[SERVER] Handling client %d\n", echoClntAddr.sin_port);
        char buf[256];
        sprintf(buf, "[SERVER] Handling client %d\n", echoClntAddr.sin_port);
        SafeLog(buf);
    }

    buffer[recvMsgSize] = '\0'; // Null terminate the string
    std::string string(buffer);
    switch (buffer[0]) {
        case 'r':
            HandleReader(sock, string);
            break;
        case 'w':
            HandleWriter(sock, string);
            break;
        case 'l':
            if (logs.find(echoClntAddr.sin_port) != logs.end()) {
                SendLogsToClient(sock, logger, logs[echoClntAddr.sin_port]);
                logs[echoClntAddr.sin_port] = logger.length();
            } else {
                logs[echoClntAddr.sin_port] = 0;
                SendLogsToClient(sock, logger, logs[echoClntAddr.sin_port]);
                logs[echoClntAddr.sin_port] = logger.length();
            }
            break;
        default:
            break;
    }

    delete[] buffer;
}

int main(int argc, char *argv[]) {
    int sock;                    /* Socket descriptor for server */
    int readers;
    int writers;
    int N;
    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr{}; /* Local address */
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
    N = writers + readers;

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    printf("Server is running on port %d. IP: %s. Waiting for connections...\n", echoServPort,
           inet_ntoa(echoServAddr.sin_addr));

    char buf[256];
    sprintf(buf, "Server is running on port %d. IP: %s. Waiting for connections...\n", echoServPort,
            inet_ntoa(echoServAddr.sin_addr));
    SafeLog(buf);
    while (true) {
        HandleUDPClient(sock);
    }

    close(sock);
    return 0;
}
