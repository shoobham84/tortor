#include "toralize.h"

Request *get_request(const int dst_port, const char *dst_ip) {
	Request *rq = malloc(sizeof(Request));
	if (rq == NULL) return NULL;

	rq->version_number = 4;
	rq->command_code = 1;

	rq->DSTPORT = htons(dst_port);
	rq->DSTIP = inet_addr(dst_ip);

	char *user_id = "toraliz";
	strncpy((char*)rq->USERID, user_id,8);

	return rq;
}

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

	Request *rq = get_request(port, host);
	if (rq == NULL) {
		perror("couldn't retrieve request packet");
		close(socket_fd);
		return EXIT_FAILURE;
	}

	if (write(socket_fd, rq, sizeof(Request)) == -1) {
		perror("error writing");
		free(rq);
		close(socket_fd);
		return EXIT_FAILURE;
	}

	char *buf = malloc(sizeof(Response));
	memset(buf, 0, sizeof(Response));

	if (read(socket_fd, buf, sizeof(Response)) == -1) {
		perror("error reading");
		free(buf);
		close(socket_fd);
		return EXIT_FAILURE;
	}

	Response *rsp = (Response*)buf;
	int response_cmd_code = rsp->command_code;

	if (response_cmd_code == 90) {
		printf("Connected successfully using proxy %s to host %s:%d\n", PROXY, host, port);
	}
	
	puts("Server connected");

	free(rq);
	free(rsp);

	close(socket_fd);
	return EXIT_SUCCESS;
}
