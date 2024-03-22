#include <cstdio>
#include <unistd.h>
#include <sys/shm.h>
#include <cstdlib>
#include <ctime>

int main() {
    // Объявляем нужные для работы параметры
    int shm_id;
    int *share;
    int num;

    // Способ №1. Ввод числа, на котором вывод закончится
    int end_num;
    printf("Введите число, на котором ввод закончится: ");
    scanf("%d", &end_num);

    // Объявляем seed у srand
    srand(time(NULL));

    // Получаем область разделяемой памяти
    shm_id = shmget(0x777, getpagesize(), 0666 | IPC_CREAT);

    // Печатаем ее ID на экран
    printf("shm_id = %d\n", shm_id);

    // Обрабатываем ошибку: что делать, если получить область не удалось
    if (shm_id < 0) {
        perror("shmget()");
        exit(1);
    }

    // Подключение области к адресному пространству процесса
    share = (int *) shmat(shm_id, 0, 0);
    if (share == NULL) {
        perror("shmat()");
        exit(2);
    }
    printf("share = %p\n", share);

    // Пока не ввели end_num
    while (true) {
        num = static_cast<int>(random() % 100);
        // проверяем, что не вывели end_num
        if (num != end_num) {
            *share = num;
            printf("Input random number %d\n", num);
            sleep(1);
        } else {
            *share = -1;
            printf("End number has been inputted!");
            break;
        }
    }
    return 0;
}
