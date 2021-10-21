// Chen Larry 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    false, true
} bool;

void Error(char *func, int file1, int file2) {
    char* message = "Error in: ";
    strcat(message, func);
    perror(message);
    if (close(file1) == -1) perror("Error in: close");
    if (close(file2) == -1) perror("Error in: close");
    exit(-1);
}

bool isEqual(int file1, int file2, int *placeToRead) {

    // buffer contains 1 char at a time
    char buf1;
    int bytes_read1 = read(file1, &buf1, 1);
    if (bytes_read1 == -1) Error("read", file1, file2);
    char buf2;
    int bytes_read2 = read(file2, &buf2, 1);
    if (bytes_read2 == -1) Error("read", file1, file2);

    // while equal chars - continue.
    while (bytes_read1 != 0 && bytes_read2 != 0 && buf1 == buf2) {
        (*placeToRead)++;

        bytes_read1 = read(file1, &buf1, 1);
        if (bytes_read1 == -1) Error("read", file1, file2);

        bytes_read2 = read(file2, &buf2, 1);
        if (bytes_read1 == -1) Error("read", file1, file2);
    }

    // if end - equals.
    if ((bytes_read1 == 0) && (bytes_read2 == 0)) {
        return true;
    }
    return false;
}

bool isSimilar(int file1, int file2, int *placeToRead) {

    // read char of file 1 and ignore everything is not a char
    char buf1;
    int bytes_read1;
    do {
        bytes_read1 = read(file1, &buf1, 1);
        if (bytes_read1 == -1) Error("read", file1, file2);
    } while ((buf1 > 126 || buf1 < 33) && bytes_read1 != 0);
    buf1 = toupper(buf1);

    // read char of file 2 and ignore everything is not a char
    char buf2;
    int bytes_read2;
    do {
        bytes_read2 = read(file2, &buf2, 1);
        if (bytes_read2 == -1) Error("read", file1, file2);
    } while ((buf2 > 126 || buf2 < 33) && bytes_read2 != 0);
    buf2 = toupper(buf2);

    while ((bytes_read1 != 0) && (bytes_read2 != 0) && (buf1 == buf2)) {
        (*placeToRead)++;

        // read char of file 1 and ignore everything is not a char
        do {
            bytes_read1 = read(file1, &buf1, 1);
            if (bytes_read1 == -1) Error("read", file1, file2);
        } while ((buf1 > 126 || buf1 < 33) && bytes_read1 != 0);
        buf1 = toupper(buf1);

        // read char of file 2 and ignore everything is not a char
        do {
            bytes_read2 = read(file2, &buf2, 1);
            if (bytes_read2 == -1) Error("read", file1, file2);
        } while ((buf2 > 126 || buf2 < 33) && bytes_read2 != 0);
        buf2 = toupper(buf2);
    }

    if ((bytes_read1 == 0) && (bytes_read2 == 0)) {
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {

    //not enough parameters
    if (argc != 3) {
        return -1;
    }

    //open files
    int file1 = open(argv[1], O_RDONLY);
    if (file1 == -1) {
        perror("Error in: open");
        exit(-1);
    }
    int file2 = open(argv[2], O_RDONLY);
    if (file2 == -1) {
        perror("Error in: open");
        close(file1);
        exit(-1);
    }

    int placeToRead = 0;

    // check if equals
    if (isEqual(file1, file2, &placeToRead) == true) {
        close(file1);
        close(file2);
        return 1;
    }

    // not equals - return 1 char backwards
    if (lseek(file1, placeToRead, SEEK_SET) == -1) Error("lseek", file1, file2);
    if (lseek(file2, placeToRead, SEEK_SET) == -1) Error("lseek", file1, file2);

    // check if similar
    if (isSimilar(file1, file2, &placeToRead) == true) {
        close(file1);
        close(file2);
        return 3;
    }

    // not equal and not similar - then different
    close(file1);
    close(file2);
    return 2;
}

