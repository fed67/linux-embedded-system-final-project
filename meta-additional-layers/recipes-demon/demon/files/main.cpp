#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        stop = 1;
    }
}

void demonize() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    pid_t sid = setsid();
    if (sid < 0) {
        perror("Error in setsid");
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        perror("Error in chdir");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main() {
    std::cout << "Simple demon!" << std::endl;

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);


    demonize();

    return 0;
}