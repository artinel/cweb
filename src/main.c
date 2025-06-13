#include "socket/server.h"
#include "version.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

static int server_fd;

__attribute__((destructor)) void destructor()
{
	server_exit(server_fd);
}

int main(int argc, char **argv)
{
	printf("CWeb version %s\n", CWEB_VERSION);

	server_fd = server_init(AF_INET, SOCK_STREAM, 0);
	if (server_fd >= 0) {
		int bind_res = server_bind(server_fd, AF_INET, INADDR_ANY,
					   DEFAULT_PORT);
		if (bind_res == 0) {
			server_listen(server_fd, DEFAULT_QUEUE_LIMIT);
		}
	}

	return 0;
}
