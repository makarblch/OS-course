#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define MULTICAST_PORT 9876
#define MULTICAST_GROUP "239.0.0.1"
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in multicastAddr;
    char buffer[BUFFER_SIZE];

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // Настройка адреса для многоадресной передачи
    memset(&multicastAddr, 0, sizeof(multicastAddr));
    multicastAddr.sin_family = AF_INET;
    multicastAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastAddr.sin_port = htons(MULTICAST_PORT);

    // Цикл отправки сообщений
    while (true) {
        std::cout << "Enter message to multicast: ";
        std::cin.getline(buffer, BUFFER_SIZE);

        // Отправка сообщения
        if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&multicastAddr, sizeof(multicastAddr)) < 0) {
            perror("sendto");
            close(sock);
            return 1;
        }
    }

    close(sock);
    return 0;
}
