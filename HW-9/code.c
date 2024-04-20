#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/sem.h>

const int NUM_MESSAGES = 10;
const int SLEEP_SECONDS = 2;

static pid_t parent_pid = -1;
static pid_t child_pid  = -1;

int fd[2];
int sem_id;

// Обработчик сигнала прерывания
void sigint_handler(int sig) {
    // Закрывает канал
    close(fd[1]);
    close(fd[0]);

    // Удаляем семафор
    if (semctl(sem_id, 0, IPC_RMID, 0) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    exit(0);
}

void child(int* fd, int sem_id) {
    char message[100];

    int i = 1;
    // Дочерний семафор
    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    while (true) {

        // Закрываем семафор для родительского процесса
        buffer.sem_op = -1;

        // Открываем семафор для дочернего процесса
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        // Читаем из пайпа
        const ssize_t bytes_read = read(fd[0], message, sizeof(message));
        if (bytes_read < 0) {
            perror("read");
            return EXIT_FAILURE;
        }

        printf("Дочерний процесс прочитал: %s\n", message);

        const int bytes_written = sprintf(message, "Сообщение: %u от дочернего[pid=%d] к родительскому[pid=%d]", i, child_pid, parent_pid);
        if (bytes_written < 0) {
            perror("sprintf");
            return EXIT_FAILURE;
        }

        if (write(fd[1], message, bytes_written + 1) < 0) {
            perror("write");
            return EXIT_FAILURE;
        }

        // Открываем семафор для родительского процесса
        buffer.sem_op = 1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        ++i;

        sleep(SLEEP_SECONDS);
    }
    return 1;
}

void parent(int * fd, int sem_id) {
    char message[100];
    int i = 1;

        // Родительский семафор
    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    while (true) {
        // Закрываем семафор для дочернего процесса
        buffer.sem_op = -1;

        // Открываем семафор для родительского процесса
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        // Читаем из пайпа
        const ssize_t bytes_read = read(fd[0], message, sizeof(message));
        if (bytes_read < 0) {
            perror("read");
            return EXIT_FAILURE;
        }

        printf("Родительский процесс прочитал: %s\n", message);

        const int bytes_written = sprintf(message, "Сообщение: %u от родительского[pid=%d] к дочернему[pid=%d]", i, parent_pid, child_pid);
        if (bytes_written < 0) {
            perror("sprintf");
            return EXIT_FAILURE;
        }

        if (write(fd[1], message, bytes_written + 1) < 0) {
            perror("write");
            return EXIT_FAILURE;
        }

        // Открываем семафор для дочернего процесса
        buffer.sem_op = 1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        ++i;

        sleep(SLEEP_SECONDS);
    }

    return 1;
}

int main() {

    // Настраиваем обработчик сигнала
    signal(SIGINT, sigint_handler);

    // Создание канала
    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    // Initialize buffer with empty content
    if (write(fd[1], "\0", 1) < 0) {
        perror("write");
        return EXIT_FAILURE;
    }
    
    // Создание и настройка семафора
   sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(-1);
    }

    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("ошибка при установке значения семафора");
        exit(-1);
    }

    parent_pid = getpid();
    child_pid = fork();

    if (child_pid < 0) {
        printf("Fork error\n");
        return -1;
    } else if (child_pid == 0) {
        child(fd, sem_id);
    } else {
        parent(fd, sem_id);
        wait(0);
        close(fd[0]);
        close(fd[1]);
        if (semctl(sem_id, 0, IPC_RMID, 0) < 0) {
            printf("Can\'t delete semaphore\n");
            return -1;
        }
    }

    return 0;
}