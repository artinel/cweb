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
#include <sys/mman.h>
#include "server.h"

static void* server_receive(void* client);
static const char* get_file_extension(const char* file_name);
static const char* get_mime_type(const char* extension);

int server_init(int socket_family, int socket_type, int protocol){

	int server_fd = socket(socket_family, socket_type, protocol);
	int opt = 1;

	if(server_fd < 0){
		printf("Failed to create server socket(%d)!!!\n", errno);
		return server_fd;
	}

	int opt_res = setsockopt(server_fd, protocol, SOL_SOCKET | SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	if(opt_res < 0){
		printf("Failed to set socket opt's(%d)\n", errno);
		return opt_res;
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
		return bind_res;
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
	int length = 0;
	while(client_fd > 0){
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
				
				FILE* file = fopen(file_path, "rb");

				if(file != NULL){
					int file_fd = fileno(file);
					struct stat file_stat;
					fstat(file_fd, &file_stat);
					
					const char* header = "HTTP/1.1 200 OK\n"\
								"Expires:-1\n";
					char file_content[file_stat.st_size + 1];
					const char* extension = get_file_extension(file_path);
					const char* mime = get_mime_type(extension);
					char* response = malloc(strlen(header) + 
							strlen(mime) + 
							file_stat.st_size + 30);

					ssize_t bytes_read = read(file_fd, file_content, file_stat.st_size);
					file_content[bytes_read] = 0;


					sprintf(response, "%sContent-Type:%s\nContent-Length:%ld\n\n", header, 
							mime, file_stat.st_size);
					
					length = strlen(response);

					memcpy(response + length, file_content, file_stat.st_size);
					

					length += file_stat.st_size;

					send(client_fd, response, length, 0);
					free(response);
					response = NULL;
					fclose(file);
					close(file_fd);

				}else{
					const char* response_404 = "HTTP/1.1 404 Not Found\n"\
					    			"Expires:-1\n"\
					    			"Content-Type:text/plain;charset=utf-8\n"\
					    			"Content-Length:13\n\n"\
					    			"404 Not Found";
					send(client_fd, response_404, strlen(response_404), 0);
				}
			}

			regfree(&regex);
		}
	}
	shutdown(client_fd, SHUT_RDWR);
	return NULL;
}

void server_exit(int server_fd){
	shutdown(server_fd, SHUT_RDWR);
}

static const char* get_file_extension(const char* file_name){
	const char* tmp = file_name;
	while(*tmp != '.' && tmp != &file_name[strlen(file_name) - 1]){
		tmp++;
	}
	if(tmp == &file_name[strlen(file_name) - 1]){
		return NULL;
	}else{
		tmp++;
	}
	return tmp;
}

static const char* get_mime_type(const char* extension){
	
	if(extension != NULL){
		if(strcasecmp(extension, "html") == 0){
			return "text/html";	
		}else if(strcasecmp(extension, "css") == 0){
			return "text/css";
		}else if(strcasecmp(extension, "js") == 0){
			return "application/javascript";
		}else if(strcasecmp(extension, "jpg") == 0){
			return "image/jpeg";
		}else if(strcasecmp(extension, "png") == 0){
			return "image/png";
		}else if(strcasecmp(extension, "mp3") == 0){
			return "audio/mpeg";
		}else if(strcasecmp(extension, "mp4") == 0){
			return "video/mp4";
		}else{
			return "application/octet-stream";
		}
	}

	return NULL;
}
