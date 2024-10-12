#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1024

struct tcp_chat_packet {
  uint16_t len;
  char message[51];
};

typedef struct tcp_chat_packet tcp_chat_packet;

#endif
