#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <random>
#include "diewitherror.cpp"
#include <csignal>

#define RCVBUFSIZE 32   /* Size of receive buffer */

std::random_device device;
auto gen = std::mt19937(device());
auto distrib = std::uniform_int_distribution<int>(0, 99);
int sock;                        /* Socket descriptor */

int generate() {
    return distrib(gen);
}

void handle_sigint(int sig) {
    printf("Caught signal %d\n", sig);
    const char *message = "q";
    send(sock, message, strlen(message), 0);
    close(sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
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
        int index = generate();
        string = "w" + std::to_string(index) + " " + std::to_string(number);
        stringLen = string.length();
        const char *cstr = string.c_str();

        /* Send the string to the server */
        if (send(sock, cstr, stringLen, 0) != stringLen)
            DieWithError("send() sent a different number of bytes than expected");

        printf("[WRITER] Sent %s to the server\n", cstr);

        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");

        echoBuffer[bytesRcvd] = '\0';  /* Terminate the string! */
        printf("[WRITER] Received: %s\n", echoBuffer);    /* Print the echo buffer */
        sleep(2); // Sleep for 1 second to prevent flooding the server
//        if (number == 0) {
//            break;
//        }
    }

    printf("[WRITER] Goodbye!\n");    /* Print a final linefeed */

    return 0;
}
