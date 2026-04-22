#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
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

void create_symlink(const char *district, int id) {
    char target[300];
    char link_name[300];
    
    // Sursa: reports.dat din interiorul folderului
    snprintf(target, sizeof(target), "reports.dat"); 
    // Numele link-ului: active_reports-<id> (va fi creat în interiorul folderului districtului)
    snprintf(link_name, sizeof(link_name), "%s/active_reports-%d", district, id);

    // Folosim symlink() - creează un link de la target la link_name
    if (symlink("reports.dat", link_name) == -1) {
        if (errno != EEXIST) perror("Eroare la crearea symlink-ului");
    } else {
        printf("Link simbolic creat: %s\n", link_name);
    }
}


void list_reports(char *district){
    char path[300];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd=open(path, O_RDONLY);
    if(fd==-1){
        perror("Eroare la deschiderea reports.dat");
        return;
    }
    Report r;
    printf("\n---Lista Rapoarte District: %s---\n", district);
    while (read(fd, &r, sizeof(Report)) > sizeof(int)) { 
        printf("[%d] Categorie: %s | Severitate: %d\n", r.id, r.Issue_Category, r.severity_level);
        printf("    Inspector: %s | GPS: %.4f, %.4f\n", r.Inspector_name, r.GPS_coordinates_x, r.GPS_coordinates_y);
        printf("    Descriere: %s\n", r.Description_text);
        printf("    Data: %s", ctime(&r.timestamp));
        printf("------------------------------------------\n");
    }
    close(fd);
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

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Utilizare: %s --role <inspector|manager> --add/--list <district>\n", argv[0]);
        return 1;
    }

    char *role = NULL;
    char *district = NULL;
    int mode = 0; // 1 pt add, 2 pt list

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) role = argv[++i];
        if (strcmp(argv[i], "--add") == 0) {
            district = argv[++i];
            mode = 1;
        }
        if (strcmp(argv[i], "--list") == 0) {
            district = argv[++i];
            mode = 2;
        }
    }

    if (!role || !district) return 1;

    if (mode == 1) {
        create_district_infrastructure(district);
    } else if (mode == 2) {
        list_reports(district);
    }

    return 0;
}