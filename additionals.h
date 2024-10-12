#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)                       \
    do {                                                       \
        if (assertion) {                                       \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
            perror(call_description);                          \
            exit(EXIT_FAILURE);                                \
        }                                                      \
    } while (0)

struct udp_msg {
    char topic[51];
    uint8_t data_type;
    char payload[1501];
} __attribute__((packed));

typedef struct udp_msg udp_msg;

struct tcp_msg {
    char ip[16];
    uint16_t port;
    char topic[51];
    char data_type[11];
    char payload[1501];
} __attribute__((packed));

typedef struct tcp_msg tcp_msg;

struct client_info {
    string id;
    int comunication_socket;
    bool is_on;
    vector<string> subscriptions;
};

typedef struct client_info client_info;

#endif