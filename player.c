#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        perror("Program invoke format: player <machine_name> <port_num>\n");
        exit(1);
    }

    // get address & port num
    const char *hostname = argv[1];
    const char *port = argv[2];
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

    // player parameters
    int socket_fd;
    int id;
    int neighbor[3] = {0, 0, 0}; // left, right, master
    char buffer[512];

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;

    // get server info
    if (getaddrinfo(hostname, port, &host_info, &host_info_list) != 0) {
        perror("Error: cannot get address info for host\n");
        exit(1);
    } //if

    // create socket
    socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (socket_fd == -1) {
        perror("Error: cannot create socket\n");
        exit(1);
    } //if

    // connect
    if (connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen) == -1) {
        perror("Error: cannot connect to socket\n");
        exit(1);
    }

    while (1) {
        if (recv(socket_fd, buffer, 512, 0) < 0) {
            printf("Error in receiving data.\n");
        } else {
            char *id = strtok(buffer, " ");
            char *n = strtok(NULL, " ");
            printf("Connected as player %s out of %s total players\n", id, n);
        }

        close(socket_fd);
        printf("Disconnected from server.\n");
        exit(1);
    }
    return 0;
}
