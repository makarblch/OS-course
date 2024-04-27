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



int sem_reader_id;
int sem_writer_id;
key_t sem_key;
char pathname[]="../egg";
int queue_id;


void cleanup() {
    semctl(sem_reader_id, 0, IPC_RMID);
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

void reader(int id) {
    srand(time(NULL) + id);
    while (1) {
        usleep(rand() % 5000000); // Simulate reading time

        sem_wait_V(sem_reader_id);
        db->reader_count++;
        if (db->reader_count == 1) {
            sem_wait_V(sem_writer_id);
        }
        sem_post_V(sem_reader_id);

        struct msgbuf {
            long mtype;
            int index;
            int value;
        };

        struct msgbuf buffer = {
                .mtype = 1,
                .index = 0,
                .value = 0
        };

        if (msgrcv(queue_id, &buffer, 8, 1, 0) != 8) {
            perror("msgrcv");
            exit (-1);
        }
        printf("Reader %d read data at index %d: %d\n", id, buffer.index, buffer.value);

        sem_wait_V(sem_reader_id);
        db->reader_count--;
        if (db->reader_count == 0) {
            sem_post_V(sem_writer_id);
        }
        sem_post_V(sem_reader_id);
    }
}

int main(int argc, char *argv[]) {
    // Signal handling
    signal(SIGINT, interrupt_handler);
    // Validate arguments

    queue_id = msgget(87, 0666 | O_CREAT);
    if (queue_id == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Create reader processes
    reader(0);

    // Cleanup
    cleanup();

    return 0;
}
