#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#define PID_FILE ".monitor_pid"

void handle_signals(int sig){
    if(sig==SIGINT){
        const char *exit_msg="\n[MONITOR] Receptionat SIGINT. Se sterge fisierul PID si se inchide \n";
        write(STDOUT_FILENO, exit_msg, strlen(exit_msg));
        unlink(PID_FILE);
        exit(0);
    }
    else if(sig==SIGUSR1){
        const char *notify_msg="[MONITOR] Notificare: Un raport nou a fost adaugat\n";
        write(STDOUT_FILENO, notify_msg, strlen(notify_msg));
    }
}




int main(){

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