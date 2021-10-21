// Chen Larry 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_CONF_SIZE 1000
#define MAX_LINE_SIZE 160

// errors
#define DIRECTORY_ERROR "Not a valid directory\n"
#define OUTPUT_ERROR "Output file not exist\n"
#define INPUT_ERROR "Input file not exist\n"

// grades
#define NO_C_FILE 0
#define COMPILATION_ERROR 10
#define TIMEOUT 20
#define WRONG 50
#define SIMILAR 75
#define EXCELLENT 100

typedef enum {
    false, true
} bool;

int Error(char *func) {
    char *message = "Error in: ";
    strcat(message, func);
    perror(message);
    return -1;
}

int cFileExist(char *fileName, char *dirPath, char *cFilePath) {

    // create user folder path
    char userPath[MAX_LINE_SIZE] = "";
    strcat(userPath, dirPath);
    strcat(userPath, "/");
    strcat(userPath, fileName);

    DIR *user = opendir(userPath);
    if (user == NULL) return Error("opendir");

    struct dirent *userIt;
    // while there is files in user folder
    while ((userIt = readdir(user)) != NULL) {
        // found suffix
        char *suffix = strrchr(userIt->d_name, '.');
        if (suffix == NULL) continue;
        // if found suffix ".c"
        if (strcmp(suffix, ".c") == 0) {
            strcat(userPath, "/");
            strcat(userPath, userIt->d_name);
            strcpy(cFilePath, userPath);
            if (closedir(user) == -1) return Error("closedir");
            return true;
        }
    }

    if (closedir(user) == -1) return Error("closedir");
    return false;
}

bool isFileCompile(char *cFilePath) {
    char *args[] = {"gcc", cFilePath, "-o", "fileRun.out", NULL};
    pid_t pid = fork();
    int status;

    if (pid < 0) {
        return Error("fork");
    }

    // child try to compile
    if (pid == 0) {
        execvp("gcc", args);
        return Error("execvp");
    }

    // father wait for child
    if (wait(&status) == -1) return Error("wait");
    // if child terminated normally
    if (WIFEXITED(status)) {
        // if there was exit code != 0
        if (WEXITSTATUS(status)) {
            return false;
        } else {
            return true;
        }
    }
    return Error("compile");
}

bool runOnTime(int inputFd) {
    // output redirection
    int outputFd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dup2(outputFd, 1) == -1) return Error("dup2");

    // set input to beginning
    if (lseek(inputFd, 0, SEEK_SET) == -1) return Error("lseek");

    char *args[] = {"./fileRun.out", NULL};
    pid_t pid = fork();
    int status;

    if (pid < 0) {
        return Error("fork");
    }

    // child try to run
    if (pid == 0) {
        alarm(5);
        execvp("./fileRun.out", args);
        return Error("execvp");
    }

    // father wait for child
    if (wait(&status) == -1) return Error("wait");
    alarm(0);
    //if child process ends with signal not handled
    if (WIFSIGNALED(status)) {
        // if the signal received was sigalrm
        if (WTERMSIG(status) == SIGALRM) {
            if (close(outputFd) == -1) return Error("close");
            return false;
        }
    }

    // if child terminated normally
    if (WIFEXITED(status)) {
        if (close(outputFd) == -1) return Error("close");
        return true;
    }

    // something failed
    if (close(outputFd) == -1) return Error("close");
    return Error("run");
}

int comp(char *outputPath) {
    char *args[] = {"./comp.out", "output.txt", outputPath, NULL};
    pid_t pid = fork();
    int status;

    if (pid < 0) {
        return Error("fork");
    }

    // child try to run
    if (pid == 0) {
        execvp("./comp.out", args);
        return Error("execvp");
    }

    // father wait for child
    waitpid(pid, &status, 0);
    //if child terminated normally
    if (WIFEXITED(status)) {
        // return the exit status of the child
        return WEXITSTATUS(status);
    }
    return Error("comp");
}

int findGrade(char *fileName, char *dirPath, char *correctOutputPath, int inputFd) {
    char cFilePath[MAX_LINE_SIZE] = "";
    if (cFileExist(fileName, dirPath, cFilePath) == false) {
        return NO_C_FILE;
    }

    if (isFileCompile(cFilePath) == false) {
        return COMPILATION_ERROR;
    }

    if (runOnTime(inputFd) == false) {
        return TIMEOUT;
    }

    int ret = comp(correctOutputPath);
    if (ret == 3) {
        return SIMILAR;
    }
    if (ret == 2) {
        return WRONG;
    }
    if (ret == 1) {
        return EXCELLENT;
    }
    return -1;
}

void writeResult(char *name, int grade, int fd) {
    char line[MAX_LINE_SIZE] = "";

    strcat(line, name);
    strcat(line, ",");

    switch (grade) {
        case NO_C_FILE:
            strcat(line, "0,NO_C_FILE");
            break;
        case COMPILATION_ERROR:
            strcat(line, "10,COMPILATION_ERROR");
            break;
        case TIMEOUT:
            strcat(line, "20,TIMEOUT");
            break;
        case WRONG:
            strcat(line, "50,WRONG");
            break;
        case SIMILAR:
            strcat(line, "75,SIMILAR");
            break;
        case EXCELLENT:
            strcat(line, "100,EXCELLENT");
            break;
        default:
            break;
    }
    strcat(line, "\n");
    write(fd, line, sizeof(line));
}

int main(int argc, char *argv[]) {
    struct stat stat_info;

    if (argc != 2) {
        return -1;
    }

    // open configuration file
    int conf = open(argv[1], O_RDONLY);
    if (conf == -1) return Error("open");
    // read configuration file into buffer
    char buffer[MAX_CONF_SIZE] = "";
    if (read(conf, buffer, MAX_CONF_SIZE - 1) == -1) {
        close(conf);
        return Error("read");
    }

    // first line - path to a directory
    char *token = strtok(buffer, "\n");
    char dirPath[MAX_LINE_SIZE] = "";
    strcpy(dirPath, token);
    // check
    if ((access(dirPath, F_OK) == -1) || (stat(dirPath, &stat_info) != -1 && !S_ISDIR(stat_info.st_mode))) {
        close(conf);
        perror(DIRECTORY_ERROR);
        return -1;
    }

    // second line - path to an input file
    token = strtok(NULL, "\n");
    char inputPath[MAX_LINE_SIZE] = "";
    strcpy(inputPath, token);
    // check
    if (access(inputPath, F_OK) == -1) {
        close(conf);
        perror(INPUT_ERROR);
        return -1;
    }

    // third line - path to a correct output file
    token = strtok(NULL, "\n");
    char correctOutputPath[MAX_LINE_SIZE] = "";
    strcpy(correctOutputPath, token);
    // check
    if (access(correctOutputPath, F_OK) == -1) {
        close(conf);
        perror(OUTPUT_ERROR);
        return -1;
    }

    // i\o redirection
    // errors
    int errorsFd = open("errors.txt", O_WRONLY | O_CREAT, 0666);
    if (errorsFd == -1) {
        close(conf);
        return Error("open");
    }
    if (dup2(errorsFd, 2) == -1) {
        close(conf);
        close(errorsFd);
        return Error("dup2");
    }
    // input
    int inputFd = open(inputPath, O_RDONLY);
    if (inputFd == -1) {
        close(conf);
        close(errorsFd);
        return Error("open");
    }
    if (dup2(inputFd, 0) == -1) {
        close(conf);
        close(errorsFd);
        close(inputFd);
        return Error("dup2");
    }
    // results
    int resultsFd = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (resultsFd == -1) {
        close(conf);
        close(errorsFd);
        close(inputFd);
        return Error("open");
    }

    // open directory
    DIR *directory = opendir(dirPath);
    if (directory == NULL) {
        close(conf);
        close(errorsFd);
        close(inputFd);
        close(resultsFd);
        return Error("opendir");
    }

    // while there is file exist in directory
    struct dirent *directoryIt;
    while ((directoryIt = readdir(directory)) != NULL) {
        char *fileName = directoryIt->d_name;
        if (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0) continue;
        // if the file is a directory
        if (directoryIt->d_type == DT_DIR) {
            int grade = findGrade(fileName, dirPath, correctOutputPath, inputFd);
            if (grade == -1) continue;
            writeResult(fileName, grade, resultsFd);
        }
    }
    if (closedir(directory) == -1) perror("Error in: closedir");
    if (close(inputFd) == -1) perror("Error in: close");
    if (close(errorsFd) == -1) perror("Error in: close");
    if (close(resultsFd) == -1) perror("Error in: close");
    remove("fileRun.out");
    remove("output.txt");
    return 0;
}


