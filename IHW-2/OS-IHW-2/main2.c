#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include "signal.h"

#define BUFFER_SIZE 100

typedef struct {
    int data[BUFFER_SIZE];
    int reader_count; // Number of active readers
} Database;

Database* db;

// Семафоры
sem_t *sem1;
sem_t *sem2;

void cleanup() {
    if (sem_close(sem1) == -1)
    {
        perror("Incorrect close of posix semaphore sem1");
        exit(-1);
    }
    if (sem_close(sem2) == -1)
    {
        perror("Incorrect close of posix semaphore sem2");
        exit(-1);
    }
    if (shm_unlink("/database") == -1) {
        perror("Incorrect unlinking of shared memory");
    }
    exit(0);
}

void interrupt_handler(int signum) {
    cleanup();
}

void reader(int id) {
    srand(time(NULL) + id);
    while (1) {
        usleep(rand() % 5000000); // Simulate reading time
        // Lock database access
        if (sem_wait(sem1) == -1) {
            perror("Sem1 wait error");
            exit(-1);
        }
        db->reader_count++;
        if (db->reader_count >= 1) {
            if (sem_wait(sem2) == -1) {
                perror("Sem2 wait error");
                exit(-1);
            }
        }
        // Unlock database access
        if (sem_post(sem1) == -1) {
            perror("Sem1 post error");
            exit(-1);
        }

        int index = rand() % BUFFER_SIZE;
        printf("Reader %d read data at index %d: %d\n", id, index, db->data[index]);

        // Lock database access
        if (sem_wait(sem1) == -1) {
            perror("Sem1 wait error");
            exit(-1);
        }
        db->reader_count--;
        if (db->reader_count == 0) { // Last reader, allow writers to write
            if (sem_post(sem2) == -1) {
                perror("Sem2 post error");
                exit(-1);
            }
        }
        // Unlock database access
        if (sem_post(sem1) == -1) {
            perror("Sem1 post error");
            exit(-1);
        }
    }
}

void writer(int id) {
    srand(time(NULL) + id);
    while (1) {
        usleep(rand() % 5000000); // Simulate writing time
        if (sem_wait(sem2) == -1) {
            perror("Sem2 wait error");
            exit(-1);
        }
        int index = rand() % BUFFER_SIZE;
        int newValue = db->data[index - 1] + rand() % (db->data[index + 1] -  db->data[index - 1]);
        printf("Writer %d wrote data at index %d: %d -> %d\n", id, index, db->data[index], newValue);
        db->data[index] = newValue;
        if (sem_post(sem2) == -1) {
            perror("Sem2 post error");
            exit(-1);
        }
    }
}

int main(int argc, char *argv[]) {
    // Signal handling
    signal(SIGINT, interrupt_handler);
    // Validate arguments
    if (argc != 3) {
        perror("Incorrect number of arguments!");
        exit(-1);
    }

    // Readers count
    int N = atoi(argv[1]);
    // Writers count
    int K = atoi(argv[2]);

    // Shared memory adress
    int shm_fd;

    // Create shared memory for the database
    if ((shm_fd = shm_open("/database", O_CREAT | O_RDWR, 0666)) == -1) {
        perror("Can't create shared memery!");
        exit(-1);
    }

    // Sizing memory
    if (ftruncate(shm_fd, sizeof(Database)) == -1) {
        perror("Memory sizing error (ftruncate)");
        exit(-1);
    }

    // Get access to the memory
    db = (Database*)mmap(0, sizeof(Database), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (db == (Database*)-1 ) {
        perror("Mmap error");
        exit(-1);
    }

    // Initialize semaphore in shared memory
    sem1 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem1 == MAP_FAILED) {
        perror("sem1 mmap error");
        exit(-1);
    }
    if (sem_init(sem1, 1, 1) == -1) {
        perror("Sem1 init error");
        exit(-1);
    }

    sem2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem2 == MAP_FAILED) {
        perror("sem2 mmap error");
        exit(-1);
    }
    if (sem_init(sem2, 1, 1) == -1) {
        perror("Sem2 init error");
        exit(-1);
    }

    // Initialize database
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        db->data[i] = i * i;
    }

    // Currently there are no readers
    db->reader_count = 0;

    // Create reader processes
    for (int i = 0; i < N; ++i) {
        if (fork() == 0) {
            reader(i);
            exit(0);
        }
    }

    // Create writer processes
    for (int i = 0; i < K; ++i) {
        if (fork() == 0) {
            writer(i);
            exit(0);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < N + K; ++i) {
        wait(NULL);
    }

    // Cleanup
    cleanup();

    return 0;
}
