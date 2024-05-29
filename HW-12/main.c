#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BASE_FILE "a"
#define SYMLINK_PREFIX "symlink_"

void cleanup(int depth) {
    char filename[256];
    for (int i = 1; i <= depth; ++i) {
        snprintf(filename, sizeof(filename), "%s%d", SYMLINK_PREFIX, i);
        unlink(filename); // Удаление символической ссылки
    }
    unlink(BASE_FILE); // Удаление основного файла
}

int main() {
    int depth = 0;
    char previous_filename[256];
    char current_filename[256];

    // Создаем базовый файл
    int fd = open(BASE_FILE, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("Failed to create base file");
        exit(EXIT_FAILURE);
    }
    close(fd);

    // Устанавливаем начальное имя файла для создания первой ссылки
    snprintf(previous_filename, sizeof(previous_filename), BASE_FILE);

    // Последовательно создаем символические ссылки и пытаемся их открыть
    while (1) {
        snprintf(current_filename, sizeof(current_filename), "%s%d", SYMLINK_PREFIX, depth + 1);
        if (symlink(previous_filename, current_filename) < 0) {
            perror("Failed to create symlink");
            break;
        }

        fd = open(current_filename, O_RDONLY);
        if (fd < 0) {
            if (errno == ELOOP) {
                printf("Maximum symlink recursion depth reached: %d\n", depth);
                break;
            } else {
                perror("Failed to open symlink");
                break;
            }
        }

        close(fd);
        depth++;

        // Обновляем имя предыдущей символической ссылки
        snprintf(previous_filename, sizeof(previous_filename), "%s%d", SYMLINK_PREFIX, depth);
    }

    cleanup(depth);
    return 0;
}
