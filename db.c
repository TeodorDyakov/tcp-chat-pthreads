#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <stdbool.h>

#include <pthread.h>


const char * DB_file = "db.txt";
pthread_mutex_t db_lock;

int initialize_db() {
    //create the db file if not exists
    FILE * fp = fopen(DB_file, "a");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    fclose(fp);

    if (pthread_mutex_init( & db_lock, NULL) != 0) {
        printf("\n mutex init failed\n");
        exit(EXIT_FAILURE);
    }
}

void save_user_info(char * username, char * password) {
    pthread_mutex_lock( & db_lock);
    FILE * fp = fopen(DB_file, "a");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    fwrite(username, strlen(username), 1, fp);
    fwrite(":", 1, 1, fp);
    fwrite(password, strlen(password), 1, fp);
    fwrite("\n", 1, 1, fp);
    fclose(fp);
    pthread_mutex_unlock( & db_lock);
}

bool check_if_user_exists(char * username, char * password) {
    pthread_mutex_lock( & db_lock);
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(DB_file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline( & line, & len, fp)) != -1) {
        char * user = strtok(line, ":");
        char * pass = strtok(NULL, ":");
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
            pthread_mutex_unlock( & db_lock);
            return true;
        }
    }
    pthread_mutex_unlock( & db_lock);
    fclose(fp);
    if (line)
        free(line);

    return false;
}