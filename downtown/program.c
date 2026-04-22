#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/typed.h>
#define MAX 200


typedef struct downtown{
    int id;
    char Inspector_name[MAX];
    float GPS_coordinates_x;
    float GPS_coordinates_y;
    char Issue_Category[MAX];
    int severity_level;
    time_t timestamp;
    char Description_text;
}Report;

int main(int argc, char *argv[]){


    return 0;
}