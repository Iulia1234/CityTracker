#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#define PID_FILE ".monitor_pid"

void handle_signals(int sig) {
    if (sig == SIGUSR1) {
        // Folosim un format clar pe care City Hub sa il recunoasca
        const char *msg = "[EVENT] New report added to system.\n";
        write(STDOUT_FILENO, msg, strlen(msg));
    } else if (sig == SIGINT) {
        const char *exit_msg = "[STATUS] Monitor closing... TERMINATED\n";
        write(STDOUT_FILENO, exit_msg, strlen(exit_msg));
        unlink(".monitor_pid");
        exit(0);
    }
}



int main(){

    int fd_check = open(".monitor_pid", O_RDONLY);
    if (fd_check != -1) {
        char existing_pid[10];
        int n = read(fd_check, existing_pid, sizeof(existing_pid) - 1);
        close(fd_check);
        if (n > 0) {
            existing_pid[n] = '\0';
            // Acest mesaj va fi citit de city_hub prin pipe!
            printf("ERROR: Monitor already running (PID: %s). TERMINATED\n", existing_pid);
            exit(1); 
        }
    }
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signals;
    sigemptyset(&sa.sa_mask); // Blocam alte semnale in timpul executiei handler-ului
    sa.sa_flags = SA_RESTART; // Restartam apelurile sistem intrerupte (ex: read)

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Eroare sigaction SIGINT");
        exit(1);
    }
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Eroare sigaction SIGUSR1");
        exit(1);
    }

    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Eroare la crearea .monitor_pid");
        exit(1);
    }
    char pid_str[16];
    int len = snprintf(pid_str, sizeof(pid_str), "%d", getpid());
    if (write(fd, pid_str, len) == -1) {
        perror("Eroare la scrierea PID-ului");
        close(fd);
        exit(1);
    }
    close(fd);
    printf("Monitor Reports a pornit (PID: %d).\n", getpid());
    printf("Astept semnale (SIGINT pentru oprire, SIGUSR1 pentru notificari)\n");

    while (1) {
        pause(); // Suspendam executia pana la venirea unui semnal
    }

    return 0;
}