#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
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
	while(1){
	ssize_t bytes_received = recv(client_fd, (void*)buffer, sizeof(buffer) / sizeof(char), 0);
	if(bytes_received > 0){

		regex_t regex;
		regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
		regmatch_t matches[2];
		if(regexec(&regex, buffer, 2, matches, 0) == 0){
			buffer[matches[1].rm_eo] = '\0';
			char* file_path = buffer;
			while(!isspace(*file_path)){
				file_path++;
			}
			file_path += 2;
			
			FILE* file = fopen(file_path, "r");
			if(file != NULL){
				struct stat file_stat;
				int fd = fileno(file);
				fstat(fd, &file_stat);
				
				const char* header = "HTTP/1.1 200 OK\n"\
							   "Content-Type:text/html;charset=utf-8\n"\
							   "Expires:-1\n";
				char response[strlen(header) + file_stat.st_size + 30];
				char file_content[file_stat.st_size + 1];
			
				int index = 0;
				char c = 0;
				while((c = fgetc(file)) != EOF){
					file_content[index++] = c;
				}
				file_content[index] = 0;

				sprintf(response, "%sContent-Length:%ld\n\n%s", header, 
						file_stat.st_size, file_content);

				send(client_fd, response, strlen(response), 0);
			}else{
				const char* response_404 = "HTTP/1.1 404 Not Found\n"\
							    "Expires:-1\n"\
							    "Content-Type:text/plain;charset=utf-8\n"\
							    "Content-Length:13\n\n"\
							    "404 Not Found";
				send(client_fd, response_404, strlen(response_404), 0);
			}
		}
	}
	}
	return NULL;
}
