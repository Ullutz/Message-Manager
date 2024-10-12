#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "additionals.h"
#include "recv_send.h"

using namespace std;

int max_poll_fds;
int max_clients;

void close_all_connections(struct pollfd *pollfds, int nfds) {
    for (int i = 1; i < nfds; i++) {
        close(pollfds[i].fd);
    }
}

client_info *search_client(vector<client_info> &clients,
                            string id, int len) {
    for (int i = 0; i < len; i++) {
        if (clients[i].id.compare(id) == 0) {
           return &clients[i];
        }
    }

    return NULL;
}

bool format_udp_msg(udp_msg *udp_msg, tcp_msg *tcp_msg) {
    switch(udp_msg->data_type) {
        case 0: {
            // sarim peste octetul de semn
            long long value = ntohl(* (long long *) (udp_msg->payload + 1));

            if (udp_msg->payload[0] != 0) {
                value *= -1;
            }

            sprintf(tcp_msg->payload, "%lld", value);
            memcpy(tcp_msg->data_type, "INT", sizeof("INT"));
            break;
        }

        case 1: {
            double value = ntohs(* (uint16_t *) (udp_msg->payload));
            value /= 100;

            sprintf(tcp_msg->payload, "%.2lf", value);
            memcpy(tcp_msg->data_type, "SHORT_REAL", sizeof("SHORT_REAL"));
            break;
        }

        case 2: {
            // sarim peste octetul de semn
            double value = ntohl( * (uint32_t *) (udp_msg->payload + 1));
            //impartim valoarea la 10 la puterea nr gasit dupa octetul de semn
            // si cei 4 octeti pt numar
            value /= pow(10, udp_msg->payload[5]);

            if (udp_msg->payload[0]) {
                value *= -1;
            }

            sprintf(tcp_msg->payload, "%f", value);
            memcpy(tcp_msg->data_type, "FLOAT", sizeof("FLOAT"));
            break;
        }

        case 3: {
            // stringurile nu mai necesita nicio preparare
            memcpy(tcp_msg->payload, udp_msg->payload,
                    sizeof(udp_msg->payload));
            memcpy(tcp_msg->data_type, "STRING", sizeof("STRING"));
            break;
        }

        default: {
            return false;
            break;
        }
    }

    return true;
}

bool match(const char *subs, const char *topic) {
    char *temp1 = strdup(topic);
    char *temp2 = strdup(subs);
    char *savep1, *savep2;

    char *p1 = strtok_r(temp1, "/", &savep1);
    char *p2 = strtok_r(temp2, "/", &savep2);

    while (p1 != NULL && p2 != NULL) {
        if (strcmp(p2, "+") == 0) {
            while (p2 != NULL && strcmp(p2, "+") == 0 && p1 != NULL) {
                p2 = strtok_r(NULL, "/", &savep2);
                p1 = strtok_r(NULL, "/", &savep1);
            }

            if (p1 != NULL && p2 == NULL) {
                return false;
            }
        } else if (strcmp(p2, "*") == 0) {
            p2 = strtok_r(NULL, "/", &savep2);

            if (p2 != NULL) {
                while (p1 != NULL && strcmp(p1, p2) != 0) {
                    p1 = strtok_r(NULL, "/", &savep1);
                }

                if (p1 == NULL) {
                    return false;
                }
            } else {
                return true;
            }
        } else {
            if (strcmp(p1, p2) != 0) {
                return false;
            } else {
                p2 = strtok_r(NULL, "/", &savep2);
                p1 = strtok_r(NULL, "/", &savep1);
            }
        }
    }

    free(temp1);
    free(temp2);

    return true;
}

bool check_for_subscription(char *topic, vector<string> subscriptions, int len) {
    for (int i = 0; i < len; i++) {
        if (match(subscriptions[i].c_str(), topic))
            return true;
    }

    return false;
}

void send_msg_to_all(tcp_msg message, vector<client_info> &clients, int len) {
    for (int i = 0; i < len; i++) {
        if (check_for_subscription(message.topic,
                                    clients[i].subscriptions,
                                    clients[i].subscriptions.size())) {
            if (clients[i].is_on) {
                int ret = send_all(clients[i].comunication_socket,
                                (char *) &message, sizeof(tcp_msg));
                DIE(ret < 0, "send fail");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 2) {
        fprintf(stderr, "Wrong type of command\n");
        return -1;
    }

    int ret;
    int port;
    int flag = 1;
    port = atoi(argv[1]);
    DIE(port == 0, "invalid port value");

    int udp_socket, listen_socket;

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_socket < 0, "socket fail");

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_socket < 0, "socket fail");

    struct sockaddr_in server_address_for_tcp, server_address_for_udp;
    
    memset((char *)&server_address_for_tcp, 0, sizeof(server_address_for_tcp));
    server_address_for_tcp.sin_family = AF_INET;
    server_address_for_tcp.sin_port = htons(port);
    server_address_for_tcp.sin_addr.s_addr = INADDR_ANY;

    ret = bind(listen_socket, (struct sockaddr *) &server_address_for_tcp,
                sizeof(struct sockaddr));
    DIE(ret < 0, "bind fail");

    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0)
        fprintf(stderr, "setsockopt");

    memset((char *)&server_address_for_udp, 0, sizeof(server_address_for_udp));
    server_address_for_udp.sin_family = AF_INET;
    server_address_for_udp.sin_port = htons(port);
    server_address_for_udp.sin_addr.s_addr = INADDR_ANY;

    ret = bind(udp_socket, (struct sockaddr *) &server_address_for_udp,
                sizeof(struct sockaddr));
    DIE(ret < 0, "bind fail");;
    
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0)
        fprintf(stderr, "setsockopt");
    
    max_poll_fds = 1000;
    struct pollfd *pollfds = (struct pollfd *)
                                calloc(max_poll_fds, sizeof(struct pollfd));
    int nfds = 0;

    pollfds[nfds].fd = STDIN_FILENO;
    pollfds[nfds].events = POLLIN;
    nfds++;

    pollfds[nfds].fd = udp_socket;
    pollfds[nfds].events = POLLIN;
    nfds++;

    pollfds[nfds].fd = listen_socket;
    pollfds[nfds].events = POLLIN;
    nfds++;

    ret = listen(listen_socket, 32);
    DIE(ret < 0, "listen fail");

    bool ok = true;

    vector<client_info> clients;
    int noclients = 0;

    while(ok) {
        ret = poll(pollfds, nfds, -1);
        DIE(ret < 0, "poll fail");

        for (int i = 0 ; i < nfds; i++) {
            if (pollfds[i].revents & POLLIN) {
                if (i == 0) {
                    // am primit comanda exit
                    char buffer[5];
                    cin >> buffer;

                    if (strcmp(buffer, "exit") == 0) {
                        ok = false;
                        break;
                    } else {
                        fprintf(stderr, "unknown command\n");
                    }
                } else if (i == 2) {
                    // primesc o cerere de conexiune la server
                    struct sockaddr_in client_address;
                    socklen_t client_addr_len = sizeof(client_address);

                    int new_connection_socket =
                            accept(listen_socket,
                                    (struct sockaddr *) &client_address,
                                    &client_addr_len);
                    DIE(new_connection_socket < 0, "accept fail");

                    int value = 1;
                    ret = setsockopt(new_connection_socket, IPPROTO_TCP,
                                        TCP_NODELAY, &value, sizeof(int));
                    DIE(ret < 0, "setsock fail");

                    uint8_t len;
                    ret = recv_all(new_connection_socket, &len,
                                sizeof(len));
                    DIE(ret < 0, "recv fail");

                    char *buffer = (char *) calloc(len, sizeof(char));

                    ret = recv_all(new_connection_socket, buffer,
                                len);
                    DIE(ret < 0, "recv fail");
                    
                    client_info *client = search_client(clients,
                                        string(buffer), noclients);
                    if (client == NULL) {
                        // daca nu gasesc niciun client cu id-ul primit
                        // accept conexiunea unui nou client
                        if (nfds >= max_poll_fds) {
                            max_poll_fds *= 2;
                            pollfds = (struct pollfd *)
                                    realloc(pollfds,
                                        max_poll_fds * sizeof(struct pollfd));
                            DIE(pollfds == NULL, "realloc fail");
                        }

                        pollfds[nfds].fd = new_connection_socket;
                        pollfds[nfds].events = POLLIN;
                        nfds++;

                        client_info new_client;
                        new_client.id = strdup(buffer);
                        new_client.is_on = true;
                        new_client.comunication_socket = new_connection_socket;

                        clients.push_back(new_client);
                        noclients++;

                        printf("New client %s connected from %s:%d.\n",
                                new_client.id.c_str(),
                                inet_ntoa(client_address.sin_addr),
                                ntohs(client_address.sin_port));
                    } else if (client->is_on == false) {
                        // daca am gasit un client cu id-ul primit
                        // il marchez ca on si refuz conexiunea noului
                        // client
                        client->is_on = true;
                        client->comunication_socket = new_connection_socket;

                        if (nfds >= max_poll_fds) {
                            max_poll_fds *= 2;
                            pollfds = (struct pollfd *)
                                    realloc(pollfds,
                                        max_poll_fds * sizeof(struct pollfd));
                            DIE(pollfds == NULL, "realloc fail");
                        }

                        pollfds[nfds].fd = new_connection_socket;
                        pollfds[nfds].events = POLLIN;
                        nfds++;

                        printf("New client %s connected from %s:%d.\n",
                                (*client).id.c_str(),
                                inet_ntoa(client_address.sin_addr),
                                ntohs(client_address.sin_port));
                        break;
                    } else {
                        printf("Client %s already connected.\n", buffer);

                        // closing the connection
                        close(new_connection_socket);
                    }
                } else if (i == 1) {
                    // am primit un mesaj de la clientii udp
                    // care trebuie formatat
                    struct sockaddr_in udp_address;
                    socklen_t udp_len = sizeof(udp_address);

                    char buffer[sizeof(udp_msg)];
                    memset(buffer, 0, sizeof(udp_msg)); 

                    ret = recvfrom(udp_socket, buffer, sizeof(buffer), 0,
                                    (struct sockaddr *)&udp_address, &udp_len);
                    DIE(ret < 0, "recv fail");

                    // populam structura mesajului udp
                    udp_msg recv_message;
                    memcpy(&recv_message.topic, buffer,
                            sizeof(recv_message.topic) - 1);
                    memcpy(&recv_message.data_type, buffer + 50,
                            sizeof(char));
                    memcpy(&recv_message.payload, buffer + 51,
                            sizeof(recv_message.payload) - 1);
                    tcp_msg to_send_msg;

                    strcpy(to_send_msg.ip,
                            inet_ntoa(udp_address.sin_addr));
                    to_send_msg.port = ntohs(udp_address.sin_port);
                    memcpy(to_send_msg.topic, recv_message.topic,
                            sizeof(recv_message.topic));

                    if (format_udp_msg(&recv_message, &to_send_msg)) {
                        send_msg_to_all(to_send_msg, clients, noclients);
                    } else {
                        fprintf(stderr, "wrong input format.\n");
                    }
                } else {
                    // s a primit mesaj pe unul din socketii tcp
                    uint8_t len;
                    ret = recv_all(pollfds[i].fd, &len, sizeof(uint8_t));
                    DIE(ret < 0, "recv fail");

                    int client_idx;
                    // caut indexul clientului care a facut comunicarea
                    for (int j = 0; j < noclients; j++) {
                        if (clients[j].comunication_socket == pollfds[i].fd) {
                            client_idx = j;
                            break;
                        }
                    }

                    if (ret == 0) {
                        printf("Client %s disconnected.\n",
                                clients[client_idx].id.c_str());

                        clients[client_idx].is_on = false;
                        clients[client_idx].comunication_socket = -1;

                        close(pollfds[i].fd);
                        for (int j = i; j < nfds - 1; j++) {
                            pollfds[j] = pollfds[j + 1];
                        }

                        nfds--;
                    } else {
                        char *buffer = (char *) calloc(len, sizeof(char));
                        DIE(buffer == NULL, "calloc");

                        ret = recv_all(pollfds[i].fd, buffer, len);
                        DIE(ret < 0, "recv fail");

                        // am primit subscribe sau unsubscribe
                        char *p = strtok(buffer, " \n");

                        if (strcmp(p, "subscribe") == 0) {
                            char *topic = strtok(NULL, " \n");

                            clients[client_idx].subscriptions.
                                    push_back(string(topic));
                        } else {
                            char *topic = strtok(NULL, " \n");

                            auto elem =
                                find(clients[client_idx].subscriptions.begin(),
                                      clients[client_idx].subscriptions.end(),
                                      string(topic));
                            
                            if (elem != clients[client_idx].subscriptions.end()) {
                                clients[client_idx].subscriptions.erase(elem);
                            } else {
                                fprintf(stderr,
                                        "Client not subscribed to this topic.\n");
                            }
                        }
                    }
                }
            }
        }
    }

    close_all_connections(pollfds, nfds);
    close(listen_socket);
    close(udp_socket);
    free(pollfds);

    return 0;
}