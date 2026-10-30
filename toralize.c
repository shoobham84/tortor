#include "toralize.h"

int main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: ./toralize <host> <port>\n");
		return EXIT_FAILURE;
	}

	char *host = argv[1];
	int port = atoi(argv[2]);


	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		perror("socket fd not found");
		return EXIT_FAILURE;
	}

	const struct sockaddr_in sock = {
		.sin_family = AF_INET,
		.sin_port = htons(PROXY_PORT),
		.sin_addr.s_addr = inet_addr(PROXY)
	};

	if (connect(socket_fd, (struct sockaddr *)&sock, sizeof(sock))) {
		perror("socket");
		return EXIT_FAILURE;
	}
	
	puts("Server connected");

	close(socket_fd);
	return EXIT_SUCCESS;
}
