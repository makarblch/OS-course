#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "diewitherror.cpp"
#include <signal.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define ECHOMAX 255

void handle_sigint(int sig) {
    printf("[LOGGER] Caught signal %d\n", sig);
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr{}; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Receive from address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    std::string string;              /* String to send to echo server */
    char echoBuffer[ECHOMAX];     /* Buffer for echo string */
    int stringLen;          /* Length of string to echo */
    int bytesRcvd;                   /* Bytes read in single recv() */
    int respStringLen;

    if ((argc < 2) || (argc > 3)) {  /* Test for correct number of arguments */
        fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];  /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]); /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);      /* Server port */

    while (true) {
        string = "l";
        stringLen = string.length();
        const char *cstr = string.c_str();

        /* Send data to the server */
        if (sendto(sock, string.c_str(), stringLen, 0, (struct sockaddr *)
                &echoServAddr, sizeof(echoServAddr)) != stringLen)
            DieWithError("sendto() sent a different number of bytes than expected");

        /* Recv a response */
        int fromSize = sizeof(fromAddr);
        if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
                                      (struct sockaddr *) &fromAddr, (socklen_t *) &fromSize)) < 0)
            DieWithError("recvfrom() failed");

        echoBuffer[respStringLen] = '\0';  /* Terminate the string! */
        if (echoBuffer[0] != '0') {
            printf("%s", echoBuffer);    /* Print the echo buffer */
        }
        sleep(1); // Sleep for 1 second to prevent flooding the server
    }

    printf("[LOGGER] Goodbye!\n");    /* Print a final linefeed */

    close(sock);
    return 0;
}
