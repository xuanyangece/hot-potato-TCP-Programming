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

    // Get server ip
    struct hostent *hent;
    hent = gethostbyname(argv[1]);
    if (NULL == hent) {
        return -1;
    }
    char * serIP;
    // get from h_addr
    serIP = inet_ntoa(*(struct in_addr*)hent->h_addr);

    // get server address
    struct sockaddr_in masterAddr;
    int count = 0;
    int PORT = atoi(argv[2]);

    // player parameters
    int player_fd;
    int id;
    char buffer[512];

    // neighbor info
    struct sockaddr_in neighborAddr[2];
    int neighbor[2] = {0, 0}; // left, right
    

    memset(&masterAddr, 0, sizeof(masterAddr));
    masterAddr.sin_family = AF_INET;
    masterAddr.sin_port = htons(PORT);
    masterAddr.sin_addr.s_addr = inet_addr(serIP); // modify later

    // create socket - self <-> ringmaster
    player_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (player_fd == -1) {
        perror("Error: cannot create socket\n");
        exit(1);
    } //if

    // connect
    if (connect(player_fd, (struct sockaddr*)&masterAddr, sizeof(masterAddr)) < 0) {
        perror("Error: cannot connect to socket\n");
        exit(1);
    }

    while (1) {
        if (count == 0 && recv(player_fd, buffer, 512, 0) < 0) {
            printf("Error in receiving data.\n");
        } else if (count == 0) {
            char *id = strtok(buffer, " ");
            char *n = strtok(NULL, " ");
            printf("Connected as player %s out of %s total players\n", id, n);
        } 

        close(player_fd);
        printf("Disconnect\n");
        exit(1);
    }
    return 0;
}
