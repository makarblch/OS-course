#include <cstdio>
#include <unistd.h>
#include <sys/shm.h>
#include <cstdlib>
#include <ctime>
#include <csignal>

pid_t another_pid;

void SIGINT_handler(int sig) {
    kill(another_pid, SIGINT);
    exit(1);
}

int main() {
    volatile int shm_id;
    volatile int *share;
    volatile int num;

    // Получаем айди для программы-клиента и -сервера
    printf("My pid, %d \n", getpid());
    printf("Pid of another program: ");
    scanf("%d", &another_pid);

    // Перенаправляем поток sigint
    signal(SIGINT, SIGINT_handler);

    srand(time(NULL));
    shm_id = shmget(0x2FF, getpagesize(), 0666 | IPC_CREAT);
    printf("shm_id = %d\n", shm_id);
    if (shm_id < 0) {
        perror("shmget()");
        exit(1);
    }

    // Подключаем область разделяемой памяти к адресному пространству программы
    share = (int *) shmat(shm_id, nullptr, 0);
    if (share == nullptr) {
        perror("shmat()");
        exit(2);
    }
    printf("share = %p\n", share);

    // Отправляем случайные числа с задержкой в 1 с.
    while (true) {
        num = static_cast<int>(random() % 1000);
        *share = num;
        printf("write a random number %d\n", num);
        sleep(1);
    }
    return 0;
}