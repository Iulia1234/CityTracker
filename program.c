#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#define MAX 200


typedef struct {
    int id;
    char Inspector_name[MAX];
    float GPS_coordinates_x;
    float GPS_coordinates_y;
    char Issue_Category[MAX];
    int severity_level;
    time_t timestamp;
    char Description_text[MAX];
}Report;

int get_next_id(const char *district) {
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    int fd = open(path, O_RDONLY);
    if (fd == -1) return 1; // Primul raport dacă fișierul nu există

    Report last_r;
    // Mergem la ultima înregistrare din fișier
    off_t pos = lseek(fd, -sizeof(Report), SEEK_END);
    
    if (pos == -1) {
        close(fd);
        return 1; // Fișier gol
    }

    read(fd, &last_r, sizeof(Report));
    close(fd);
    return last_r.id + 1;
}

// Notifică programul monitor_reports prin SIGUSR1
void notify_monitor() {
    int fd = open(".monitor_pid", O_RDONLY);
    if (fd == -1) return; // Monitorul nu rulează

    char pid_str[10];
    int n = read(fd, pid_str, sizeof(pid_str) - 1);
    close(fd);

    if (n > 0) {
        pid_str[n] = '\0';
        pid_t monitor_pid = atoi(pid_str);
        kill(monitor_pid, SIGUSR1); // Trimite semnalul USR1
    }
}



void create_symlink(const char *district) {
    char target[300], link_name[300];
    snprintf(target, sizeof(target), "%s/reports.dat", district);
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district); 
    
    unlink(link_name); 
    if (symlink(target, link_name) == -1) {
        if (errno != EEXIST) perror("Eroare symlink");
    } else {
        printf("Link simbolic creat: %s -> %s\n", link_name, target);
    }
}


void create_district_infrastructure(char *dname){
    if(mkdir(dname,0750)==-1){
        perror("Eroare la crearea directorului\n");
        return;
    }

    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", dname);
    int fd=open(path, O_CREAT | O_RDWR, 0664);
    if(fd==-1){
        perror("Eroare la crearea reports.dat");
        return;
    }
    else{
        chmod(path, 0664);
        close(fd);
    }

    snprintf(path, sizeof(path), "%s/district.cfg", dname);
    fd=open(path, O_CREAT | O_RDWR, 0640);
    if(fd!=-1){
        chmod(path, 0640);
        close(fd);
    }

    snprintf(path, sizeof(path), "%s/logged_district", dname);
    fd=open(path, O_CREAT | O_RDWR, 0644);
    if(fd!=-1){
        chmod(path, 0644);
        close(fd);
    }

    printf("Districtul %s a fost configurat.\n", dname);
}

void get_permissions_string(mode_t mode, char *str) {
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

int check_access(const char *path, const char *role, mode_t required_bit) {
    struct stat st;
    if (stat(path, &st) == -1) return 0;

    if (strcmp(role, "manager") == 0) {
        mode_t owner_bit = 0;
        if (required_bit & S_IRUSR) owner_bit |= S_IRUSR;
        if (required_bit & S_IWUSR) owner_bit |= S_IWUSR;
        return (st.st_mode & owner_bit);
    } else if (strcmp(role, "inspector") == 0) {
        mode_t group_bit = 0;
        if (required_bit & S_IRUSR) group_bit |= S_IRGRP;
        if (required_bit & S_IWUSR) group_bit |= S_IWGRP;
        return (st.st_mode & group_bit);
    }
    return 0;
}

void add_report(const char *district, const char *user, const char *role) {
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    // Obținem ID-ul automat[cite: 1]
    int next_id = get_next_id(district);

    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului de rapoarte");
        return;
    }

    Report r;
    r.id = next_id;
    strncpy(r.Inspector_name, user, MAX); 
    r.timestamp = time(NULL);

    // Citire date de la tastatură[cite: 1]
    printf("\n--- Adaugare Raport Nou (ID: %d) ---\n", r.id);
    
    printf("Coordonata GPS X (Latitudine): ");
    scanf("%f", &r.GPS_coordinates_x);
    
    printf("Coordonata GPS Y (Longitudine): ");
    scanf("%f", &r.GPS_coordinates_y);

    printf("Categorie (ex: road, lighting, flooding): ");
    scanf("%s", r.Issue_Category);

    printf("Nivel Severitate (1-3): ");
    scanf("%d", &r.severity_level);

    getchar(); // Consumă newline-ul rămas în buffer
    printf("Descriere text: ");
    fgets(r.Description_text, MAX, stdin);
    r.Description_text[strcspn(r.Description_text, "\n")] = 0; // Elimină \n de la final

    write(fd, &r, sizeof(Report));
    chmod(path, 0664);
    close(fd);

    printf("Raportul %d a fost salvat cu succes.\n", r.id);
    
    // Notificăm monitorul pentru Faza 2
    notify_monitor();
}

void list_reports(char *district) {
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    
    struct stat st;
    if (stat(path, &st) == -1) return;

    char perm_str[10];
    get_permissions_string(st.st_mode, perm_str); 

    printf("Fisier: reports.dat | Permisiuni: %s | Dimensiune: %ld bytes | Modificat: %s", 
    perm_str, st.st_size, ctime(&st.st_mtime));

}

void view_report(const char *district, int report_id) {
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd = open(path, O_RDONLY);
    if (fd == -1) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == report_id) {
            printf("Detalii complete Raport %d:\n", r.id);
            printf("Categorie: %s\nSeveritate: %d\nDescriere: %s\n", 
                   r.Issue_Category, r.severity_level, r.Description_text);
            close(fd);
            return;
        }
    }
    printf("Raportul %d nu a fost gasit.\n", report_id);
    close(fd);
}

void update_threshold(const char *district, int value, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Acces refuzat: Doar managerul poate schimba pragul.\n");
        return;
    }

    char path[300];
    snprintf(path, sizeof(path), "%s/district.cfg", district);

    struct stat st;
    if (stat(path, &st) == 0) {
        if ((st.st_mode & 0777) != 0640) {
            printf("Alerta securitate: Permisiunile district.cfg au fost modificate! Operatiune refuzata.\n");
            return;
        }
    }

    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "threshold=%d\n", value);
        fclose(f);
        printf("Prag actualizat la %d.\n", value);
    }
}

void remove_report(const char *district, int report_id, const char *role) {
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Eroare: Doar managerul poate sterge rapoarte.\n");
        return;
    }

    int fd = open(path, O_RDWR);
    if (fd == -1) return;

    Report r;
    int found = 0;
    off_t pos;

    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.id == report_id) {
            found = 1;
            pos = lseek(fd, 0, SEEK_CUR) - sizeof(Report);
            
            Report next;
            while (read(fd, &next, sizeof(Report)) > 0) {
                lseek(fd, pos, SEEK_SET);
                write(fd, &next, sizeof(Report));
                pos += sizeof(Report);
                lseek(fd, pos + sizeof(Report), SEEK_SET);
            }
            
            struct stat st;
            fstat(fd, &st);
            ftruncate(fd, st.st_size - sizeof(Report));
            printf("Raportul %d a fost eliminat.\n", report_id);
            break;
        }
    }
    if (!found) printf("Raportul nu a fost gasit.\n");
    close(fd);
}

void remove_district(char *district, const char *role){
    char path[300];
    if(strcmp(role, "manager")!=0){
        fprintf(stderr, "Eroare: Doar managerul poate sterge districte\n");
        return;
    }
    snprintf(path, sizeof(path), "active_reports-%s", district);
    unlink(path);
    pid_t pid=fork();
    if(pid<0){
        perror("Eroare la fork");
        return;
    }
    if(pid==0){
        if(strlen(district)==0 || strcmp(district, ".")==0 || strcmp(district, "/")==0){
            fprintf(stderr, "Eroare: Nume district invalid pentru stergere\n");
            exit(EXIT_FAILURE);
        }
        printf("[FIU] Se sterge directorul: %s\n", district);
        char* argv[]={"rm", "-rf", district, NULL};
        execvp("rm", argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else{
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status)==0){
            printf("Districtul '%s' a fost eliminat cu succes de catre procesul %d\n", district, pid);
        }
        else{
            printf("Procesul de stergere a esuat\n");
        }
    }

}

int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[100]; 
    strcpy(temp, input); 

    
    char *token = strtok(temp, ":");
    if (token == NULL) return 0;
    strcpy(field, token);

    
    token = strtok(NULL, ":");
    if (token == NULL) return 0;
    strcpy(op, token);


    token = strtok(NULL, ":");
    if (token == NULL) return 0;
    strcpy(value, token);

    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    int val_int = atoi(value);
    
    if (strcmp(field, "severity") == 0) {
        if (strcmp(op, "==") == 0) return r->severity_level == val_int;
        if (strcmp(op, ">=") == 0) return r->severity_level >= val_int;
        if (strcmp(op, "<=") == 0) return r->severity_level <= val_int;
    }
    if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->Issue_Category, value) == 0;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char *role = NULL, *user = "unknown", *district = NULL, *command = NULL;
    int report_id = -1, threshold_val = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0) user = argv[++i];
        else if (strcmp(argv[i], "--add") == 0) { district = argv[++i]; command = "add"; }
        else if (strcmp(argv[i], "--list") == 0) { district = argv[++i]; command = "list"; }
        else if (strcmp(argv[i], "--view") == 0) { district = argv[++i]; report_id = atoi(argv[++i]); command = "view"; }
        else if (strcmp(argv[i], "--remove_report") == 0) { district = argv[++i]; report_id = atoi(argv[++i]); command = "remove"; }
        else if (strcmp(argv[i], "--update_threshold") == 0) { district = argv[++i]; threshold_val = atoi(argv[++i]); command = "threshold"; }
        else if (strcmp(argv[i], "--remove_district")==0){district = argv[++i]; command = "remove_district";}
    }

    if (!role || !district || !command) {
        printf("Utilizare: %s --role <role> --user <user> <command> <district> [args]\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(district, &st) == -1) {
        mkdir(district, 0750);
        char p[300];
        snprintf(p, sizeof(p), "%s/reports.dat", district); close(open(p, O_CREAT, 0664));
        snprintf(p, sizeof(p), "%s/district.cfg", district); close(open(p, O_CREAT, 0640));
        snprintf(p, sizeof(p), "%s/logged_district", district); close(open(p, O_CREAT, 0644));
    }

    if (strcmp(command, "add") == 0) {
        add_report(district, user, role);
        create_symlink(district); 
    } else if (strcmp(command, "list") == 0) {
        list_reports(district);
    } else if (strcmp(command, "view") == 0) {
        view_report(district, report_id);
    } else if (strcmp(command, "remove") == 0) {
        remove_report(district, report_id, role);
    } else if (strcmp(command, "threshold") == 0) {
        update_threshold(district, threshold_val, role);
    } else if (strcmp(command, "remove_district")==0) {
        remove_district(district, role);
    }

    return 0;
}