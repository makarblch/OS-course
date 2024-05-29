#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <random>
#include "diewitherror.cpp"

#define RCVBUFSIZE 32   /* Size of receive buffer */

std::random_device device;
auto gen = std::mt19937(device());
auto distrib = std::uniform_int_distribution<int>(0, 100);

int generate() {
    return distrib(gen);
}

int main(int argc, char *argv[]) {
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr{}; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    std::string string;              /* String to send to echo server */
    char echoBuffer[RCVBUFSIZE];     /* Buffer for echo string */
    unsigned int stringLen;          /* Length of string to echo */
    int bytesRcvd;                   /* Bytes read in single recv() */

    if ((argc < 2) || (argc > 3)) {  /* Test for correct number of arguments */
        fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];  /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]); /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);      /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    while (string != "0") {
        int number = generate();
        string = "r" + std::to_string(number);
        stringLen = string.length();
        const char *cstr = string.c_str();

        /* Send the string to the server */
        if (send(sock, cstr, stringLen, 0) != stringLen)
            DieWithError("send() sent a different number of bytes than expected");

        printf("[READER] Sent %s to the server\n", cstr);

        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");

        echoBuffer[bytesRcvd] = '\0';  /* Terminate the string! */
        printf("[READER] Received: %s\n", echoBuffer);    /* Print the echo buffer */
        sleep(2); // Sleep for 1 second to prevent flooding the server
    }

    printf("[READER] Goodbye!\n");    /* Print a final linefeed */

    close(sock);
    return 0;
}
