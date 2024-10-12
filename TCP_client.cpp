#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <iostream>

#include "additionals.h"
#include "recv_send.h"

using namespace std;

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int socket_fd, ret, nfds = 0;
    struct sockaddr_in server_address;
    struct pollfd pfds[2];

    if (argc < 4) {
        fprintf(stderr, "Wrong format of command");
        return -1;
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socket_fd < 0, "eroare socket");

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &server_address.sin_addr);
    DIE(ret <= 0, "inet");

    ret = connect(socket_fd, (struct sockaddr *) &server_address,
                    sizeof(server_address));
    DIE(ret < 0, "Connect failed");

    // putem primi comenzi de la stdin
    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    // sau putem primi mesaje de la server pe socket
    pfds[nfds].fd = socket_fd;
    pfds[nfds].events = POLLIN;
    nfds++;

    uint8_t len = strlen(argv[1]) + 1;
    ret = send_all(socket_fd, &len, sizeof(len));
    DIE(ret < 0, "send failed");

    // trimit ID-ul serverului
    ret = send_all(socket_fd, argv[1], strlen(argv[1]) + 1);
    DIE(ret < 0, "send failed");

    // dezactivez alg lui Nagle
    int value = 1;
    ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY,
                        (char *) &value, sizeof(int));
    DIE(ret < 0, "failed to deactivate nagle alg");

    while (1) {
        ret = poll(pfds, nfds, -1);
        DIE(ret < 0, "poll error");

        if (pfds[0].revents && POLLIN) {
            // primesc mesaj de la stdin
            // si ii verific intefritate
            char buffer[100];
            fgets(buffer, sizeof(buffer), stdin);

            char *c_buffer = strdup(buffer);
            char *p = strtok(c_buffer, " \n");

            int good_message = 1;
            char *topic;

            if (strcmp(p, "subscribe") == 0) {
                p = strtok(NULL, " \n");
                if (p == NULL) {
                    fprintf(stderr, "Topic not found.\n");
                    good_message = 0;
                }
                
                topic = p;

                p = strtok(NULL, " \n");
                if (p != NULL) {
                    fprintf(stderr, "Too many arguments for "
                                        "subscribe command.\n");
                    good_message = 0;
                }
            } else if (strcmp(p, "unsubscribe") == 0) {
                p = strtok(NULL, " \n");
                if (p == NULL) {
                    fprintf(stderr, "Topic not found.\n");
                    good_message = 0;
                }

                topic = p;

                p = strtok(NULL, " \n");
                if (p != NULL) {
                    fprintf(stderr, "Too many arguments for "
                                        "unsubscribe command.\n");
                    good_message = 0;
                }
            } else if (strcmp(p, "exit") == 0) {
                break;
            } else {
                fprintf(stderr, "unknown command\n");
                good_message = 0;
            }

            if (good_message) {
                // mesajul are formatul corect
                // asa ca ii trimit mai intai lungimea
                uint8_t len = strlen(buffer);
                ret = send_all(socket_fd, &len, sizeof(len));
                DIE(ret < 0, "send failed");

                // si apoi trimit si mesajul in sine
                ret = send_all(socket_fd, buffer, strlen(buffer));
                DIE(ret < 0, "send failed");

                if (strstr(buffer, "unsubscribe") != NULL) {
                    printf("Unsubscribed from topic %s.\n", topic);
                } else {
                    printf("Subscribed to topic %s.\n", topic);
                }
            }

            free(c_buffer);
        } else if (pfds[1].revents && POLLIN) {
            // primesc un mesaj de la server
            char buffer[sizeof(tcp_msg)];
            memset(buffer, 0, sizeof(tcp_msg));

            ret = recv_all(socket_fd, buffer, sizeof(tcp_msg));
            DIE(ret < 0, "recv failed");

            // serverul a fost inchis
            // sau conexiunea a fost respinsa
            if (ret == 0) {
                break;
            }

            tcp_msg *message = (tcp_msg *)buffer;
            printf("%s:%d - %s - %s - %s\n", message->ip,
                    message->port, message->topic, message->data_type,
                    message->payload);
        }
    }

    close(socket_fd);

    return 0;
}