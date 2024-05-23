#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define MULTICAST_PORT 9876
#define MULTICAST_GROUP "239.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in localAddr, groupAddr;
    char buffer[BUFFER_SIZE];
    struct ip_mreq group;

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // Настройка адреса для приема многоадресных сообщений
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(MULTICAST_PORT);

    // Привязка сокета
    if (bind(sock, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    // Настройка для присоединения к многоадресной группе
    group.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
        perror("setsockopt");
        close(sock);
        return 1;
    }

    // Цикл приема сообщений
    while (true) {
        socklen_t addrLen = sizeof(groupAddr);
        int recvLen = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&groupAddr, &addrLen);
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
