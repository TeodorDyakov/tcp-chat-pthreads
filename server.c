#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
#include "db.h"

#define MAX_MSG_LEN 512
#define PORT 8080
#define MAX_CONNECTIONS 512
#define SA struct sockaddr
pthread_mutex_t lock;
pthread_mutex_t db_lock;

int connections[MAX_CONNECTIONS];
char* usernames[MAX_CONNECTIONS];
int num_connections = 0;

typedef struct thread_ctx
{
	int fd;
	int idx;
	char *username;
	char *address;
}
thread_ctx;

bool starts_with(const char *a, const char *b)
{
	if(strncmp(a, b, strlen(b)) == 0) return 1;
	return 0;
}

void *handle_client(void *void_ptr)
{
	thread_ctx th_ctx = *(thread_ctx*) void_ptr;
	int sockfd = th_ctx.fd;
	int idx = th_ctx.idx;
	char *ip_address = th_ctx.address;
	char *username = th_ctx.username;

	pthread_mutex_lock(&lock);
	connections[num_connections] = sockfd;
	num_connections++;
	pthread_mutex_unlock(&lock);

	char buff[MAX_MSG_LEN];
	bool logged_in = false;

	for (;;)
	{
		bzero(buff, MAX_MSG_LEN);
		int n_read = read(sockfd, buff, sizeof(buff) - 1);

		if (n_read <= 0){
			free(void_ptr);
			pthread_mutex_lock(&lock);
			close(sockfd);
			connections[idx] = -1;
			pthread_mutex_unlock(&lock);
			break;
		}

		if(!logged_in){	
			if(starts_with(buff, "/login")){
				strtok(buff, " ");
				char* user = strtok(NULL, " ");
				char* pass = strtok(NULL, " ");
				//remove new line at end of string
				pass[strlen(pass) - 1] = 0;

				pthread_mutex_lock(&db_lock);

				if(check_if_user_exists(user, pass)){
					pthread_mutex_unlock(&db_lock);
				
					logged_in = true;
					char* msg = "Logged in succesfully!\n";
					
					pthread_mutex_lock(&lock);

					th_ctx.username = malloc(strlen(user) + 1);
					strcpy(th_ctx.username, user);					
					usernames[idx] = th_ctx.username;

					pthread_mutex_unlock(&lock);

					write(sockfd, msg, strlen(msg));
				}else{
					pthread_mutex_unlock(&db_lock);
					char* msg = "No such user found!\n";
					write(sockfd, msg, strlen(msg));
				}
			}else if(starts_with(buff, "/register")){
				strtok(buff, " ");
				char* user = strtok(NULL, " ");
				char* pass = strtok(NULL, " ");
				//remove new line at end of string
				pass[strlen(pass) - 1] = 0;

				pthread_mutex_lock(&db_lock);
				
				if(check_if_user_exists(user, pass)){
					pthread_mutex_unlock(&db_lock);
				
					char* msg = "User already exists!\n";
					write(sockfd, msg, strlen(msg));
				}else{
					pthread_mutex_unlock(&db_lock);
					logged_in = true;
					char* msg = "Registered and Logged in succesfully!\n";

					pthread_mutex_lock(&lock);

					th_ctx.username = malloc(strlen(user) + 1);
					strcpy(th_ctx.username, user);
					usernames[idx] = th_ctx.username;
					pthread_mutex_unlock(&lock);

					write(sockfd, msg, strlen(msg));
					pthread_mutex_lock(&db_lock);
					save_user_info(user, pass);
					pthread_mutex_unlock(&db_lock);
				}
			}else{
				continue;
			}
		}else if(starts_with(buff, "/sendmsg") || starts_with(buff, "/sendmsgto")){
			pthread_mutex_lock(&lock);
			char* recipient = NULL;
			bool is_private_msg = false;
			
			char *msg = NULL;
			strtok(buff, "\"");
			msg = strtok(NULL, "\"");
			
			if(starts_with(buff, "/sendmsgto")){
					strtok(buff, " ");
					recipient = strtok(NULL, " ");
					is_private_msg = true;
			}
			//debug
			printf("Message is %s", msg);

			for (int i = 0; i < num_connections; i++){
				if (connections[i] == -1){
					continue;
				}
				if(!is_private_msg || (is_private_msg && strcmp(usernames[i], recipient) == 0)){
					time_t t = time(NULL);
					struct tm *tm = localtime(&t);
					char s[64];
					strftime(s, sizeof(s), "%c", tm);
					
					const int FULL_MSG_LEN = 1000;
					char *full_msg = (char*)malloc(FULL_MSG_LEN);
					bzero(full_msg, FULL_MSG_LEN);
					if(!is_private_msg){
						sprintf(full_msg, "%s %s :%s\n", th_ctx.username, s, msg);
					}else{
						sprintf(full_msg, "(Private message)%s %s:%s\n", th_ctx.username, s, msg);
					}
					//for debug
					printf("%s\n", full_msg);

					write(connections[i], full_msg, strlen(full_msg));					
				}

			}
			pthread_mutex_unlock(&lock);
		}
	}
}
	
int main()
{

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*) &servaddr, sizeof(servaddr))) != 0)
	{
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0)
	{
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

	int num_connections = 0;

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}

	if (pthread_mutex_init(&db_lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}

	pthread_t th[MAX_CONNECTIONS];

	for (;;)
	{
		connfd = accept(sockfd, (SA*) &cli, &len);

		if (connfd < 0)
		{
			printf("server acccept failed...\n");
			continue;
		}
		else
		{
			printf("server acccept the client...\n");

			struct in_addr ipAddr = cli.sin_addr;
			char *str = malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);

			thread_ctx *th_ctx = malloc(sizeof(thread_ctx));
			th_ctx->fd = connfd;
			th_ctx->address = str;
			th_ctx->idx = num_connections;
			pthread_create(&th[num_connections], NULL, &handle_client, th_ctx);
		}
	}
}
