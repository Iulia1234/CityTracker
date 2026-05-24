#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

void calculate_scores_command(int argc, char *argv[]){
    // Numarul real de districte este totalul minus primele 2 argumente (./p si --calculate_scores)
    int count = argc - 2; 
    
    pid_t pids[10];
    int pipe_fds[10];

    // Pornim bucla de la i = 2 pentru a lua districtele din argv
    for(int i = 2; i < argc; i++){
        int idx = i - 2; // Index salvat pentru vectorii interni pids si pipe_fds (0, 1, 2...)

        int pfd[2];
        if(pipe(pfd) == -1){ perror("Eroare pipe"); return; }
        
        pid_t pid = fork();
        if(pid < 0) { perror("Eroare fork"); return; }
        
        if(pid == 0){
            close(pfd[0]);
            dup2(pfd[1], STDOUT_FILENO);
            close(pfd[1]);

            // Trimite argv[i], care acum va fi garantat "central", "cub" sau "upt"
            execl("./scorer", "scorer", argv[i], NULL);
            perror("Eroare execl scorer");
            exit(1);
        }
        else{
            close(pfd[1]);
            pipe_fds[idx] = pfd[0];
            pids[idx] = pid;
        }
    }

    printf("\n=== COMBINED WORKLOAD REPORT ===\n");
    for(int i = 0; i < count; i++){
        char buffer[512];
        int n;

        while((n = read(pipe_fds[i], buffer, sizeof(buffer) - 1)) > 0){
            buffer[n] = '\0';
            printf("%s", buffer);
        }
        close(pipe_fds[i]);
        waitpid(pids[i], NULL, 0);
    }
    printf("================================\n");
}

void start_monitor_command(){
    int pfd[2];
    if(pipe(pfd)<0){
        perror("Eroare la crearea pipe-ului!\n");
        exit(1);
    }

    // Creeaza procesul de fundal hub_mon
    pid_t hub_mon_pid = fork();
    if(hub_mon_pid<0){
        perror("Eroare la fork");
        exit(1);
    }
    
    if(hub_mon_pid==0){ 
        // Suntem in hub_mon
        close(pfd[0]); // Nu citeste din teava

        // hub_mon face un al doilea fork pentru a lansa monitorul efectiv
        pid_t monitor_pid = fork();
        if(monitor_pid == 0) {
            // Suntem in procesul monitor
            dup2(pfd[1], STDOUT_FILENO); // Trimite stdout in teava
            close(pfd[1]);

            char *argv[] = {"./monitor_reports", "monitor", NULL};
            execvp(argv[0], argv);
            perror("execvp esuat");
            exit(EXIT_FAILURE);
        }
        
        // Tot hub_mon inchide copia lui de scriere ca sa poata functiona EOF
        close(pfd[1]); 
        waitpid(monitor_pid, NULL, 0); // Asigura curatarea monitorului
        exit(0);
    } else {
        // Suntem in parinte (city_hub)
        close(pfd[1]); // Inchide scrierea ca sa primeasca EOF de la citire
        char buffer[256];
        int n;
        printf("[HUB] Se porneste monitorul. Asteptare mesaje...\n");

        // Citeste mesajele initiale trimise de monitor prin teava
        while((n = read(pfd[0], buffer, sizeof(buffer)-1)) > 0){
            buffer[n]='\0';
            printf("%s", buffer);
            // Daca citim ca s-a terminat/e deja pornit, iesim din bucla
            if (strstr(buffer, "TERMINATED")) {
                break;
            }
        }
        close(pfd[0]);
        // Nu dam wait aici ca sa lasam hub_mon sa ruleze monitorul in background
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Utilizare:\n");
        printf("  %s --start_monitor\n", argv[0]);
        printf("  %s --calculate_scores <district1> <district2> ...\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--start_monitor") == 0) {
        start_monitor_command();
    } 
    else if (strcmp(argv[1], "--calculate_scores") == 0) {
    if (argc < 3) {
        printf("Eroare: Trebuie sa specifici cel putin un district dupa --calculate_scores\n");
        return 1;
    }
    // Trimitem direct argc și argv-ul original
    calculate_scores_command(argc, argv);
    }
    else {
        printf("Comanda necunoscuta: %s\n", argv[1]);
        return 1;
    }
    return 0;
}