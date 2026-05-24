#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 200
#define MAX_INSPECTORS 100

// Structura identica cu cea din program.c pentru citirea binara
typedef struct {
    int id;
    char Inspector_name[MAX];
    float GPS_coordinates_x;
    float GPS_coordinates_y;
    char Issue_Category[MAX];
    int severity_level;
    long timestamp;
    char Description_text[MAX];
} Report;

// Liste pentru stocarea agregata a scorurilor per inspector
char names[MAX_INSPECTORS][MAX];
int scores[MAX_INSPECTORS];
int inspector_count = 0;

void add_to_score(const char *name, int severity) {
    // Cauta daca inspectorul exista deja in lista noastra
    for (int i = 0; i < inspector_count; i++) {
        if (strcmp(names[i], name) == 0) {
            scores[i] += severity;
            return;
        }
    }
    // Daca e nou, adauga-l in tabel
    if (inspector_count < MAX_INSPECTORS) {
        strcpy(names[inspector_count], name);
        scores[inspector_count] = severity;
        inspector_count++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Scorer are nevoie de numele districtului.\n");
        return 1;
    }

    char *district = argv[1];
    char path[350];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("District [%s]: Nu s-a putut deschide reports.dat sau nu exista rapoarte.\n", district);
        return 0;
    }

    Report r;
    // Citeste fisierul binar structura cu structura
    while (read(fd, &r, sizeof(Report)) > 0) {
        add_to_score(r.Inspector_name, r.severity_level);
    }
    close(fd);

    // Printeaza rezultatele (ajung direct in pipe-ul bunicului prin dup2)
    printf("Raport District [%s]:\n", district);
    if (inspector_count == 0) {
        printf("  Fara rapoarte valide gasite.\n");
    }
    for (int i = 0; i < inspector_count; i++) {
        printf("  -> Inspector: %s | Workload Score: %d\n", names[i], scores[i]);
    }
    return 0;
}