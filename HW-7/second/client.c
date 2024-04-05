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
    shm_id = shmget("HW-8", O_CREAT | O_RDWRT, 0666);
    printf("shm_id = %d\n", shm_id);
    if (shm_id < 0) {
        perror("shmget()");
        exit(1);
    }

   // Задание размера объекта памяти
    if (ftruncate(shm_id, sizeof(num)) == -1) {
        perror("ftruncate");
        return 1;
    } else {
        printf("Memory size set and = %lu\n", sizeof(num));
    }

    /* подключение сегмента к адресному пространству процесса */
    share = static_cast<int *>(mmap(nullptr, sizeof(num), PROT_WRITE | PROT_READ, MAP_SHARED, shm_id, 0));
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