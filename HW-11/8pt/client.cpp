#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BROADCAST_PORT 9876
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in recvAddr;
    char buffer[BUFFER_SIZE];
    socklen_t addrLen = sizeof(recvAddr);

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // Настройка адреса для получения сообщений
    memset(&recvAddr, 0, sizeof(recvAddr));
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    recvAddr.sin_port = htons(BROADCAST_PORT);

    // Привязка сокета
    if (bind(sock, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    // Цикл приема сообщений
    while (true) {
        int recvLen = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&recvAddr, &addrLen);
        if (recvLen < 0) {
            perror("recvfrom");
            close(sock);
            return 1;
        }

        buffer[recvLen] = '\0';
        std::cout << "Received message: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}
