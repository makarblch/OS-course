#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define BROADCAST_PORT 9876
#define BROADCAST_IP "255.255.255.255"
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in broadcastAddr;
    char buffer[BUFFER_SIZE];
    int broadcastPermission = 1;

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // Разрешение широковещательной передачи
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0) {
        perror("setsockopt");
        close(sock);
        return 1;
    }

    // Настройка адреса для широковещательной передачи
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    broadcastAddr.sin_port = htons(BROADCAST_PORT);

    // Цикл отправки сообщений
    while (true) {
        std::cout << "Enter message to broadcast: ";
        std::cin.getline(buffer, BUFFER_SIZE);

        // Отправка сообщения
        if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
            perror("sendto");
            close(sock);
            return 1;
        }
    }

    close(sock);
    return 0;
}