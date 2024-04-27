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
#include <sys/msg.h>

int sem_reader_id;
int sem_writer_id;
char pathname[]="../egg";
int queue_id;

static int create_semaphore_or_increase(key_t sem_key,
                                        uint32_t sem_initial_value) {
    int sem_id        = semget(sem_key, 1, NEW_SEMAPHORE_OPEN_FLAGS);
    const bool failed = sem_id == -1 && errno != EEXIST;
    if (failed) {
        perror("semget");
        return sem_id;
    }

    const bool sem_already_exists = sem_id == -1;
    if (sem_already_exists) {
        sem_id = semget(sem_key, 1, EXISTING_SEMAPHORE_OPEN_FLAGS);
    }

    if (sem_id == -1) {
        perror("semget");
        return sem_id;
    }

    if (!sem_already_exists) {
        printf("Created new semaphore[sem_id=%d]\n", sem_id);
        if (semctl(sem_id, 0, SETVAL, sem_initial_value) == -1) {
            perror("semctl");
            if (semctl(sem_id, 0, IPC_RMID) == -1) {
                perror("shmctl");
            } else {
                printf(
                        "Could not init semaphore value. Deleted "
                        "semaphore[sem_id=%d]\n",
                        sem_id);
            }
            return -1;
        }
    } else {
        printf("Opened existing semaphore[sem_id=%d]\n", sem_id);
        struct sembuf buffer = {
                .sem_num = 0,
                .sem_op  = 1,
                .sem_flg = 0,
        };
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return -1;
        }
    }

    return sem_id;
}


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

        struct msgbuf {
            long mtype;
            int index;
            int value;
        };

        struct msgbuf buffer = {
                .mtype = 1,
                .index = index,
                .value = value
        };

        if (msgsnd(queue_id, &buffer, 8, 0) != 8) {
            perror("msgsnd");
            exit(-1);
        }
        printf("Writer %d wrote data at index %d: %d\n", id, index, value);

        sem_post_V(sem_writer_id);
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

    key_t sem_key = 95958;
    create_semaphore_or_increase(sem_key, 1);
    // Create writer processes
    writer(0);

    // Cleanup
    cleanup();

    return 0;
}
