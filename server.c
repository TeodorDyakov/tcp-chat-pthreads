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
#include <signal.h>
#define MAX 512
#define PORT 8080
#define MAX_CONN 128
#define SA struct sockaddr
pthread_mutex_t lock;

int connections[128];
int num_connections = 0;

typedef struct thread_ctx
{
	int fd;
	int idx;
	char *address;
}
thread_ctx;

void handler(int num)
{
	return;
};

void *handle_client(void *void_ptr)
{
	thread_ctx th_ctx = *(thread_ctx*) void_ptr;
	int sockfd = th_ctx.fd;
	int idx = th_ctx.idx;
	char *ip_address = th_ctx.address;

	pthread_mutex_lock(&lock);
	connections[num_connections] = sockfd;
	num_connections++;
	pthread_mutex_unlock(&lock);

	char buff[MAX];

	for (;;)
	{
		bzero(buff, MAX);
		int n_read = read(sockfd, buff, sizeof(buff) - 1);

		if (n_read <= 0)
		{
			pthread_mutex_lock(&lock);
			close(sockfd);
			connections[idx] = -1;
			pthread_mutex_unlock(&lock);
			break;
		}

		pthread_mutex_lock(&lock);

		for (int i = 0; i < num_connections; i++)
		{
			if (connections[i] == -1)
			{
				continue;
			}
			write(connections[i], ip_address, strlen(ip_address));

			time_t t = time(NULL);
			struct tm *tm = localtime(&t);
			char s[64];

			strftime(s, sizeof(s), "%c", tm);
			write(connections[i], " ", 1);
			write(connections[i], s, sizeof(s));
			write(connections[i], ":", 1);

			write(connections[i], buff, strlen(buff));
		}
		pthread_mutex_unlock(&lock);
	}
}

// Driver function
int main()
{
	signal(SIGPIPE, handler);

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

	pthread_t th[128];
	int num_connections = 0;

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}

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
			int *sockfd = malloc(sizeof(int));
			th_ctx->fd = sockfd;
			th_ctx->address = str;
			th_ctx->idx = *(int*) malloc(sizeof(int));
			pthread_create(&th[num_connections], NULL, &handle_client, th_ctx);
		}
	}
}
