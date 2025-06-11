#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include "server.h"

static void* server_receive(void* client);

int server_init(int socket_family, int socket_type, int protocol){

	int server_fd = socket(socket_family, socket_type, protocol);
	int opt = 1;

	if(server_fd < 0){
		printf("Failed to create server socket(%d)!!!\n", errno);
		return errno;
	}

	int opt_res = setsockopt(server_fd, protocol, SOL_SOCKET | SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	if(opt_res < 0){
		printf("Failed to set socket opt's(%d)\n", errno);
		return errno;
	}

	return server_fd;									      
}

int server_bind(int server_fd, int addr_family, uint32_t addr, uint16_t port){
	struct sockaddr_in server_addr;
	server_addr.sin_family = addr_family;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = addr;

	int bind_res = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(bind_res < 0){
		printf("Failed to bind address and port to socket(%d)!!!\n", errno);
		return errno;
	}
	return 0;
}

int server_listen(int server_fd, uint32_t limit){
	int listen_res = listen(server_fd, limit);
	if(listen_res < 0){
		printf("Failed to listen to socket(%d)!!!\n", errno);
		return errno;
	}
	while(1){
		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
		if(client_fd < 0){
			printf("Failed to accept client(%d)!!!\n", errno);
			continue;
		}
		pthread_t thread;
		pthread_create(&thread, NULL, server_receive, (void*)&client_fd);
		pthread_detach(thread);
	}
	return 0;
}

static void* server_receive(void* client){
	int client_fd = *((int*)client);
	char buffer[DEFAULT_BUFFER_SIZE];
	ssize_t bytes_received = recv(client_fd, (void*)buffer, sizeof(buffer) / sizeof(char), 0);
	if(bytes_received > 0){
		printf("%s\n\n\n\n\n", buffer);
	}
	return NULL;
}
