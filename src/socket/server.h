#ifndef __SERVER_H_
#define __SERVER_H_

#include <stdint.h>

#define DEFAULT_PORT 6754
#define DEFAULT_QUEUE_LIMIT 10
#define DEFAULT_BUFFER_SIZE 2048

int server_init(int socket_family, int socket_type, int protocol);
int server_bind(int server_fd, int addr_family, uint32_t addr, uint16_t port);
int server_listen(int server_fd, uint32_t limit);

#endif
