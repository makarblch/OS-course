#include <iostream>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// buffer size = 32 bytes
const int size = 32;

int main(int argc, char *argv[]) {
    int fd1;
    int fd2;
    char buffer[size + 1];
    std::string filename1;
    std::string filename2;
    ssize_t read_bytes;

    if (argc == 3) {
        filename1 = argv[1];
        filename2 = argv[2];
    } else {
        printf("Incorrect number of arguments!\n");
        return -1; // incorrect number of arguments
    }

    // Get file descriptor for reading
    if ((fd1 = open(filename1.c_str(), O_RDONLY)) < 0) {
        printf("Can\'t open file\n");
        exit(-1);
    }

    // Get file descriptor for writing
    if((fd2 = open(filename2.c_str(), O_WRONLY | O_CREAT,
                   0666)) < 0){
        printf("Can\'t open file\n");
        exit(-1);
    }

    // reading 32 bytes of read. file and writing them to wr. file then
    do {
        read_bytes = read(fd1, buffer, size);
        if (read_bytes == -1) {
            printf("Can\'t write this file\n");
            exit(-1);
        }
        buffer[read_bytes] = '\0';
        // Можно выводить в терминал содержимое файла
        // printf("%s", buffer);
        write(fd2, buffer, read_bytes);
    } while (read_bytes == size);

    // struct to get access rights
    struct stat st;
    // get read. file access mode
    fstat(fd1, &st);
    // changing wr. file access mode
    fchmod(fd2, st.st_mode);

    // close read. file descriptor
    if (close(fd1) < 0) {
        printf("Can\'t close file\n");
    }

    // close wr. file descriptor
    if (close(fd2) < 0) {
        printf("Can\'t close file\n");
    }

    return 0;
}
