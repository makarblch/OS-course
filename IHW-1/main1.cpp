#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUFFER_SIZE 5000

bool containsElement(const char arr[], char element) {
    for (int i = 0; i < 6; i++) {
        if (arr[i] == element) {
            return true;
        }
    }
    return false;
}

char *replaceVowels(char buffer[], ssize_t number) {
    char vowels[] = {'a', 'e', 'i', 'u', 'o', 'y'};
    for (int i = 0; i < number; ++i) {
        if (containsElement(vowels, buffer[i])) {
            buffer[i] -= 32;
        }
    }
    return buffer;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Incorrect number of arguments!");
        exit(-1);
    }

    char *inputFile = argv[1];
    char *outputFile = argv[2];

    // Defining named pipes
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    // If fifo already exist
    remove(fifo1);
    remove(fifo2);

    // Creating named pipes (FIFOs)
    if (mkfifo(fifo1, S_IFIFO | 0666) < 0 || mkfifo(fifo2,  S_IFIFO | 0666) < 0) {
        printf("Fifo creating error");
        exit(-1);
    }

    pid_t pid = getpid();

    // Fork process 1 to read from the file
    pid_t pid1 = fork();

    if (pid1 == -1) {
        printf("Fork creating error");
        exit(-1);
    } else if (pid1 == 0) {
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

        // Open fifo1 for writing
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

        // Closing file descriptor and pipe to this process
        close(inputFileDescriptor);
        close(fifoFd1);
        exit(0);
    }

    // Fork process 2
    pid_t pid2 = fork();
    if (pid2 == -1) {
        printf("Fork creating error");
        exit(-1);
    } else if (pid2 == 0) {
        // Process 2: Read from fifo1, process data, write to fifo2
        // Open fifo1 for reading
        int fifoFd1 = open(fifo1, O_RDONLY);
        if (fifoFd1 == -1) {
            printf("Fifo1 open error");
            exit(-1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead = read(fifoFd1, buffer, BUFFER_SIZE);

        if (bytesRead == -1) {
            printf("Read from fifo1 error");
            exit(-1);
        }

        // Apply function to buffer
        char *modify = replaceVowels(buffer, bytesRead);

        char result[BUFFER_SIZE];
        // Converting char* to char[]
        strncpy(result, modify, sizeof(result));
        result[sizeof(result) - 1] = '\0';

        int fifoFd2 = open(fifo2, O_WRONLY);
        if (fifoFd1 == -1) {
            printf("Fifo2 open error");
            exit(-1);
        }

        // Write char[] to second pipe
        if (write(fifoFd2, result, strlen(result)) < 0) {
            printf("Write to fifo2 error");
            exit(-1);
        }

        // Closing pipes for this process
        close(fifoFd1);
        close(fifoFd2);
        exit(0);
    }

    // Fork process 3
    pid_t pid3 = fork();
    if (pid3 == -1) {
        printf("Fork creating error");
        exit(-1);
    } else if (pid3 == 0) {
        // Process 3: Read from fifo2, write to output file
        int fifoFd2 = open(fifo2, O_RDONLY);
        if (fifoFd2 == -1) {
            printf("Fifo2 open error");
            exit(-1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead = read(fifoFd2, buffer, BUFFER_SIZE);

        if (bytesRead == -1) {
            printf("Read from fifo2 error");
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
        exit(0);
    }

    // Wait for child processes to finish
    waitpid(-1, NULL, 0);

    // Unlinking FIFOs in order to stop their existence
    unlink(fifo1);
    unlink(fifo2);
    return 0;
}