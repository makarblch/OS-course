#ifndef DATABASE_H
#define DATABASE_H

#define BUFFER_SIZE 100

typedef struct {
    int data[BUFFER_SIZE];
    int reader_count; // Number of active readers
} Database;

#endif /* DATABASE_H */
