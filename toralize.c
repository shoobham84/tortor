#include "toralize.h"

static char target_domain[256];

Request *get_request(struct sockaddr_in* sock2) {
	Request *rq = malloc(sizeof(Request));
	if (rq == NULL) return NULL;

	rq->version_number = 4;
	rq->command_code = 1;

	rq->DSTPORT = sock2->sin_port;
	rq->DSTIP = sock2->sin_addr.s_addr;

	char *user_id = "toraliz";
	strncpy((char*)rq->USERID, user_id, 8);

	return rq;
}

bool is_domain(const char* domain) {
	if (!domain) return false;
	regex_t regex;
	
	// domain regex
	const char *pattern = "^([a-zA-Z0-9-]{1,63}\\.)+[a-zA-Z]{2,63}$";

	if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		return false;
	}
	int re_match = regexec(&regex, domain, 0, NULL, 0);
	regfree(&regex);

	return (re_match == 0);
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
	int (*ptr)(const char *, const char *, const struct addrinfo *, struct addrinfo **); 
	ptr = dlsym(RTLD_NEXT, "getaddrinfo");

	if (!node)
		return ptr(node, service, hints, res);

	struct in_addr dummy;
	if (inet_aton(node, &dummy)) 
		return ptr(node, service, hints, res);

	if (is_domain(node)) {
		if (hints && hints->ai_family == AF_INET6) {
			return EAI_ADDRFAMILY;
		}

		strncpy(target_domain, node, sizeof(target_domain) - 1);
		target_domain[sizeof(target_domain) - 1] = '\0';

		struct addrinfo *addrinfo_struct = calloc(1, sizeof(struct addrinfo));
		struct sockaddr_in *sadrin_struct = calloc(1, sizeof(struct sockaddr_in));

		sadrin_struct->sin_family = AF_INET;
		if (service) {
			int p = atoi(service);
			if (p > 0) {
				sadrin_struct->sin_port = htons(p);
			} else {
				struct servent *se = getservbyname(service, "tcp");
				sadrin_struct->sin_port = se ? se->s_port : htons(80);
			}
		} else {
			sadrin_struct->sin_port = 0;
		}
		sadrin_struct->sin_addr.s_addr = inet_addr(FAKE_IP);

		addrinfo_struct->ai_family = AF_INET;
		addrinfo_struct->ai_socktype = hints ? hints->ai_socktype : SOCK_STREAM;
		addrinfo_struct->ai_addrlen = sizeof(struct sockaddr_in);
		addrinfo_struct->ai_addr = (struct sockaddr *)sadrin_struct;
		*res = addrinfo_struct;
		return 0;
	}

	return ptr(node, service, hints, res);
}

void freeaddrinfo(struct addrinfo *res) {
	void (*orig_freeaddrinfo)(struct addrinfo *);
	orig_freeaddrinfo = dlsym(RTLD_NEXT, "freeaddrinfo");

	if (res && res->ai_addr && res->ai_addr->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
		if (sin->sin_addr.s_addr == inet_addr(FAKE_IP)) {
			free(res->ai_addr);
			free(res);
			return;
		}
	}
	orig_freeaddrinfo(res);
}

int connect(int sockfd_2, const struct sockaddr *sockaddr_2, socklen_t addrlen)
{
	int (*p)(int, const struct sockaddr *, socklen_t);
	p = dlsym(RTLD_NEXT, "connect");

	if (sockaddr_2->sa_family == AF_INET6) {
		// Reject IPv6 so apps fall back to IPv4 (prevents IPv6 leaks)
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (sockaddr_2->sa_family != AF_INET) 
		return p(sockfd_2, sockaddr_2, addrlen);

	struct sockaddr_in *target = (struct sockaddr_in*)sockaddr_2;

	// if loopback connection: connect locally (except for our FAKE_IP)
	if ((ntohl(target->sin_addr.s_addr) >> 24) == 127 && target->sin_addr.s_addr != inet_addr(FAKE_IP)) 
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
		close(socket_fd);
		return EXIT_FAILURE;
	}

	char *packet = NULL;
	size_t packet_size = 0;

	if (target->sin_addr.s_addr == inet_addr(FAKE_IP)) {
		// SOCKS4a 
		packet_size = sizeof(Request) + strlen(target_domain) + 1;
		packet = malloc(packet_size);
		if (!packet) {
			close(socket_fd);
			return EXIT_FAILURE;
		}

		Request *rq = (Request*)packet;
		rq->version_number = 4;
		rq->command_code = 1;
		rq->DSTPORT = target->sin_port;
		rq->DSTIP = inet_addr("0.0.0.1");	// SOCKS4a marker 
		strncpy((char*)rq->USERID, "toraliz", 8);

		char *domain_ptr = packet + sizeof(Request);
		strcpy(domain_ptr, target_domain);
	}
	else {
		packet_size = sizeof(Request);
		packet = (char*)get_request(target);
		if (!packet) {
			close(socket_fd);
			return EXIT_FAILURE;
		}
	}

	if (write(socket_fd, packet, packet_size) == -1) {
		perror("error writing");
		free(packet);
		close(socket_fd);
		return EXIT_FAILURE;
	}
	free(packet);

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
		fprintf(stderr, "Proxy connection rejected: error code %d\n", response_cmd_code);
		close(socket_fd);
		return -1;
	}

	dup2(socket_fd, sockfd_2);
	close(socket_fd);

	return EXIT_SUCCESS;
}
