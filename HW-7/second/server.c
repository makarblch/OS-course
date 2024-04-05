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
    shm_id = shmget("HW-8", O_CREAT | O_RDWRT, 0666);
    if (shm_id == -1) {
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

    // Принимаем числа с задеркой в 1 с.
    while (true) {
        sleep(1);
        printf("got from shared memory: %d\n", *share);
    }

    return 0;
}