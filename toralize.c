#include "toralize.h"

Request *get_request(struct sockaddr_in* sock2) {
	Request *rq = malloc(sizeof(Request));
	if (rq == NULL) return NULL;

	rq->version_number = 4;
	rq->command_code = 1;

	rq->DSTPORT = sock2->sin_port;
	rq->DSTIP = sock2->sin_addr.s_addr;

	char *user_id = "toraliz";
	strncpy((char*)rq->USERID, user_id,8);

	return rq;
}

 int connect(int sockfd_2, const struct sockaddr *sockaddr_2, socklen_t addrlen)
{
	int (*p)(int, const struct sockaddr *, socklen_t);
	p = dlsym(RTLD_NEXT, "connect");

	if (sockaddr_2->sa_family != AF_INET) 
		return p(sockfd_2, sockaddr_2, addrlen);

	struct sockaddr_in *target = (struct sockaddr_in*)sockaddr_2;

	// if loopback connection: connect locally
	if ((ntohl(target->sin_addr.s_addr) >> 24) == 127) 
		return p(sockfd_2, sockaddr_2, addrlen);

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

	if (p(socket_fd, (struct sockaddr *)&sock, sizeof(sock))) {
		perror("socket");
		return EXIT_FAILURE;
	}

	Request *rq = get_request((struct sockaddr_in*)sockaddr_2);
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

	char buf[sizeof(Response)];
	memset(buf, 0, sizeof(Response));

	if (read(socket_fd, buf, sizeof(Response)) == -1) {
		perror("error reading");
		close(socket_fd);
		return EXIT_FAILURE;
	}

	Response *rsp = (Response*)buf;
	int response_cmd_code = rsp->command_code;

	if (response_cmd_code != 90) {
		fprintf(stderr, "Proxy connectin rejected: error code %d\n", response_cmd_code);
		close(socket_fd);
		free(rq);
		return -1;
	}
	
	puts("Server connected");

	dup2(socket_fd, sockfd_2);

	close(socket_fd);
	free(rq);

	return EXIT_SUCCESS;
}
