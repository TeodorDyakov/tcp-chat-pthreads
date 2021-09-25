#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

const char* DB_file = "db.txt";

void save_user_info(char* username, char* password){
    FILE* fp = fopen(DB_file, "a");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    fwrite("\n", 1, 1, fp);
    fwrite(username, strlen(username), 1, fp);
    fwrite(":", 1, 1, fp);
    fwrite(password, strlen(password), 1, fp);
    fclose(fp);
}

bool check_if_user_exists(char* username, char* password){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(DB_file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        char * user = strtok(line, ":");
        char* pass = strtok(NULL, ":");
        if(strcmp(user, username) == 0 && strcmp(pass, password) == 0){
            return true;
        }
    }

    fclose(fp);
    if (line)
        free(line);
    return false;
}
