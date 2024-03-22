#include <cstdio>
#include <unistd.h>
#include <sys/shm.h>
#include <cstdlib>

// Signal handler
void SIGINT_handler(int sig) {
    exit(1);
}

int main() {
    int shm_id;
    int *share;

    // Получаем айди для программы-клиента и -сервера
    printf("My pid, %d \n", getpid());
    pid_t another_pid;
    printf("Pid of another program: ");
    scanf("%d", &another_pid);

    // Настраиваем обработчик сигналов
    signal(SIGINT, SIGINT_handler);
 
    // Получаем область разделяемой памяти
    shm_id = shmget(0x2FF, getpagesize(), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget()");
        exit(1);
    }

    // Подключаем область разделяемой памяти к адресному пространству программы
    share = (int *) shmat(shm_id, nullptr, 0);
    if (share == nullptr) {
        perror("shmat()");
        exit(2);
    }

    // Принимаем числа с задеркой в 1 с.
    while (true) {
        sleep(1);
        printf("got from shared memory: %d\n", *share);
    }

    return 0;
}