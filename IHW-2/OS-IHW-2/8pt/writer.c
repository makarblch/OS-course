#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "database.h"

Database* db; // Объявляем здесь

int sem_reader_id;
int sem_writer_id;
key_t shm_key;
char pathname[]="../egg";

void cleanup() {
    semctl(sem_writer_id, 0, IPC_RMID);
    shmdt(db);
    exit(EXIT_SUCCESS);
}

void interrupt_handler(int signum) {
    cleanup();
}

void sem_wait_V(int sem_id) {
    struct sembuf sem_op_wait = {0, -1, SEM_UNDO};
    if (semop(sem_id, &sem_op_wait, 1) == -1) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

void sem_post_V(int sem_id) {
    struct sembuf sem_op_post = {0, 1, SEM_UNDO};
    if (semop(sem_id, &sem_op_post, 1) == -1) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

void writer(int id) {
    srand(time(NULL) + id);
    while (1) {
        usleep(rand() % 5000000); // Simulate writing time

        sem_wait_V(sem_writer_id);

        int index = rand() % BUFFER_SIZE;
        int value = rand() % 1000;
        db->data[index] = value;
        printf("Writer %d wrote data at index %d: %d\n", id, index, value);

        sem_post_V(sem_writer_id);
    }
}

int main(int argc, char *argv[]) {
    // Signal handling
    signal(SIGINT, interrupt_handler);
    // Validate arguments
    if (argc != 2) {
        perror("Incorrect number of arguments!");
        exit(EXIT_FAILURE);
    }

    // Writers count
    int N = atoi(argv[1]);

    int shmid;

    shm_key = ftok(pathname, 0);

    if ((shmid = shmget(shm_key, sizeof(Database), 0666 | IPC_CREAT | IPC_EXCL)) < 0)  {
        if ((shmid = shmget(shm_key, sizeof(Database), 0)) < 0) {
            perror("shmget");
            exit(EXIT_FAILURE);
        }
        db = (Database*)shmat(shmid, NULL, 0);
    } else {
        db = (Database*)shmat(shmid, NULL, 0);
        for(int i = 0; i < BUFFER_SIZE; ++i) {
            db->data[i] = 0;
        }
    }

    // Create writer semaphore
    sem_writer_id = semget(shm_key + 1, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (sem_writer_id == -1) {
        if ((sem_writer_id = semget(shm_key + 1, 1, 0666)) == -1) {
            perror("semget (writer)");
            exit(EXIT_FAILURE);
        }
    } else {
        if (semctl(sem_writer_id, 0, SETVAL, 1) == -1) {
            perror("semctl (writer)");
            exit(EXIT_FAILURE);
        }
    }

    // Create reader semaphore
    sem_reader_id = semget(shm_key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (sem_reader_id == -1) {
        if ((sem_reader_id = semget(shm_key, 1, 0666)) == -1) {
            perror("semget (reader)");
            exit(EXIT_FAILURE);
        }
    } else {
        if (semctl(sem_reader_id, 0, SETVAL, 1) == -1) {
            perror("semctl (reader)");
            exit(EXIT_FAILURE);
        }
    }

    // Create writer processes
    writer(0);

    // Wait for all child processes to finish
    for (int i = 0; i < N; ++i) {
        wait(NULL);
    }

    // Cleanup
    cleanup();

    return 0;
}
