#include <sys/sem.h>
#include <sys/ipc.h>
#include <errno.h>
#include <stdbool.h>

const int OPEN_FLAGS_EXIST = 0666;
const int OPEN_FLAGS_NEW = 0666 | IPC_CREAT | IPC_EXCL;

inline int create_semaphore_or_increase(key_t sem_key,
                                        uint32_t sem_initial_value) {
    int sem_id = semget(sem_key, 1, OPEN_FLAGS_NEW);
    const bool failed = sem_id == -1 && errno != EEXIST;
    if (failed) {
        perror("semget");
        return sem_id;
    }

    const bool sem_already_exists = sem_id == -1;
    if (sem_already_exists) {
        sem_id = semget(sem_key, 1, OPEN_FLAGS_EXIST);
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


inline void delete_semaphore(int sem_id) {
    if (sem_id == -1) {
        return;
    }
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
    } else {
        printf("Deleted semaphore[sem_id=%d]\n", sem_id);
    }
}

inline void delete_queue(int queue_id) {
    if (queue_id == -1) {
        return;
    }
    if (msgctl(queue_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
    } else {
        printf("Deleted queue[queue_id=%d]\n", queue_id);
    }
}

void deinit_queue(int queue_id, int sem_id) {

    if (sem_id == -1 || queue_id == -1) {
        return;
    }

    struct sembuf buffer = {
            .sem_num = 0,
            .sem_op  = -1,
            .sem_flg = IPC_NOWAIT,
    };
    if (semop(sem_id, &buffer, 1) == -1) {
        bool counter_is_zero = errno == EAGAIN;
        if (counter_is_zero) {
            return;
        }

        perror("semop");
    }
    switch (semctl(sem_id, 0, GETVAL)) {
        case -1:
            perror("semctl");
            break;
        case 0:
            delete_semaphore(sem_id);
            delete_queue(queue_id);
            break;
        default:
            break;
    };
}
