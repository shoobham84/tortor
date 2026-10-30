#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PROXY "127.0.0.1"
#define PROXY_PORT 9050

/*		CONNECT Reply Packet	*/

typedef struct _Rqst {
	uint8_t version_number = 4;
	uint8_t command_code = 1;  // for CONNECT
	uint16_t DSTPORT;
	uint32_t DSTIP;
	unsigned char USERID[8];
} Request;

typedef struct _Rsp {
	uint8_t version_number = 0;
	uint8_t command_code;  // 90 -> success; 91 -> rejected; 92 -> inability to connect on client; 93 -> rejected due to different user id
	uint16_t DSTPORT;	/* ignored */
	uint32_t DSTIP;		/* ignored */
} Response;
