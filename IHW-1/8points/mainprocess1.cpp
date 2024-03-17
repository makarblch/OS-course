#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUFFER_SIZE 5000

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Incorrect number of arguments!");
        exit(-1);
    }

    char *inputFile = argv[1];
    char *outputFile = argv[2];

    // Defining named fifos
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    // In case they already exist
    remove(fifo1);
    remove(fifo2);

    // Creating named pipes (fifos)
    if (mkfifo(fifo1, S_IFIFO | 0666) < 0 || mkfifo(fifo2, S_IFIFO | 0666) < 0) {
        perror("Fifo creating error");
        exit(-1);
    }

    // This is a main process
    int inputFileDescriptor = open(inputFile, O_RDONLY);
    if (inputFileDescriptor < 0) {
        printf("Open input file error");
        exit(-1);
    }
    // Creating buffer to read buffer_size bytes
    char buffer[BUFFER_SIZE];
    // Read file
    ssize_t bytesRead = read(inputFileDescriptor, buffer, BUFFER_SIZE);
    if (bytesRead < 0) {
        printf("Read input file error");
        exit(-1);
    }

    // C-style string ends with \0
    buffer[bytesRead] = '\0';

    // Open first fifo to write
    int fifoFd1 = open(fifo1, O_WRONLY);
    if (fifoFd1 == -1) {
        printf("Fifo1 open error");
        exit(-1);
    }

    // Writing buffer to pipe
    if (write(fifoFd1, buffer, bytesRead) < 0) {
        printf("Write to fifo1 error");
        exit(-1);
    }

    close(inputFileDescriptor);
    close(fifoFd1);

    int fifoFd2 = open(fifo2, O_RDONLY);
    if (fifoFd2 == -1) {
        printf("Fifo2 open error");
        exit(-1);
    }
    // Read from the second fifo
    bytesRead = read(fifoFd2, buffer, BUFFER_SIZE);

    if (bytesRead == -1) {
        printf("Read from pipe2 error");
        exit(-1);
    }
    // Write to output file
    int outputFileDescriptor = open(outputFile, O_WRONLY | O_CREAT,
                                    0666);
    if (outputFileDescriptor == -1) {
        printf("Open output file error");
        exit(-1);
    }

    if (write(outputFileDescriptor, buffer, bytesRead) < 0) {
        printf("Write to output file error");
        exit(-1);
    }

    close(outputFileDescriptor);
    close(fifoFd2);

    unlink(fifo1);
    unlink(fifo2);
}