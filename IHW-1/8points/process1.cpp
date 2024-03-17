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

int main() {

    // Defining named fifos
    const char *fifo1 = "fifo1";
    const char *fifo2 = "fifo2";

    // Open first fifo to read
    int fifoFd1 = open(fifo1, O_RDONLY);
    if (fifoFd1 == -1) {
        printf("Fifo1 open error");
        exit(-1);
    }

    // open second fifo to write
    int fifoFd2 = open(fifo2, O_WRONLY);
    if (fifoFd1 == -1) {
        printf("Fifo2 open error");
        exit(-1);
    }

    // Read from fifo1, process data, write to fifo2
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = read(fifoFd1, buffer, BUFFER_SIZE);

    if (bytesRead == -1) {
        printf("Read from pipe1 error");
        exit(-1);
    }

    // Apply function to buffer
    char *modify = replaceVowels(buffer, bytesRead);

    char result[BUFFER_SIZE];
    // Converting char* to char[]
    strncpy(result, modify, sizeof(result));
    result[sizeof(result) - 1] = '\0';

    // Write char[] to second pipe
    if (write(fifoFd2, result, strlen(result)) < 0) {
        printf("Write to pipe2 error");
        exit(-1);
    }

    // Closing pipes for this process
    close(fifoFd1);
    close(fifoFd2);
}