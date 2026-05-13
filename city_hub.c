#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>



int main(){

    int pfd[2];

    if(pipe(pfd)<0){
        perror("Eroare la crearea pipe-ului!\n");
        exit(1);
    }

    pid_t hub_mon_pid = fork();
    if(hub_mon_pid<0){
        perror("Eroare la fork");
        exit(1);
    }
    if(hub_mon_pid==0){
        close(pfd[0]);


        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]);
        char *argv[] = {"./monitor_reports", "monitor", NULL};
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
        close(pfd[1]);
        wait(NULL);
        exit(0);
    }else{
        close(pfd[1]);
        char buffer[256];
        int n;
        const char *prefix_msg="\n[HUB] Pornire monitor si asteptare mesaje initiale: \n";
        write(STDOUT_FILENO, prefix_msg, strlen(prefix_msg));

        while((n = read(pfd[0], buffer, sizeof(buffer)-1))>0){
            buffer[n]='\0';
            write(STDOUT_FILENO, buffer, sizeof(buffer)-1);
            if (strstr(buffer, "TERMINATED")) break;
        }
        close(pfd[0]);

    }




    return 0;
}