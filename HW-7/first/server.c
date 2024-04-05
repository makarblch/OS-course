#include <cstdio>
#include <unistd.h>
#include <sys/shm.h>
#include <cstdlib>

int main() {
    int shm_id;
    int *share;

    // Открываем доступ ту же самую область разделяемой памяти
    shm_id = shmget("HW-8", O_CREAT | O_RDWRT, 0666);
    if (shm_id == -1) {
        perror("shmget()");
        exit(1);
    }
  
    // Включение области разделяемой памяти в адресное пространство текущего процесса
    share = static_cast<int *>(mmap(nullptr, sizeof(int), PROT_WRITE | PROT_READ, MAP_SHARED, shm_id, 0));
    if (share == nullptr) {
        perror("shmat()");
        exit(2);
    }

    // с задержкой в 1 секунду пытаемся взять число из разделяемой памяти
    while (true) {
        sleep(1);
        if (*share != -1) {
            printf("got from shared memory: %d\n", *share);
        } else {
            printf("end of process! exit");
            break;
        }
    }

    return 0;
}