#include<stdbool.h>
void initialize_db();
void save_user_info(char* username, char* password);
bool check_if_user_exists(char* username, char* password);