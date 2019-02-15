#include <arpa/inet.h>
#include <netdb.h> // gethostbyname
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {
  // Get input
  if (argc != 4) {
    perror("Program invoke format: ringmaster <port_num> <num_players> "
           "<num_hops>\n");
    exit(1);
  }

  int PORT = atoi(argv[1]);
  int N = atoi(argv[2]);
  int hops = atoi(argv[3]);

  if (N <= 1 || hops < 0 || hops > 2048) {
    perror("Invalid num_players or num_hops\n");
    exit(1);
  }

  // socket variables
  int master_fd, ret;
  struct sockaddr_in masterAddr;
  fd_set readfds;

  int playerID = 0;
  int *player_fd = malloc(sizeof(*player_fd) * N); // newSocket
  int *player_PORT = malloc(sizeof(*player_PORT) * N);
  struct sockaddr_in *playerAddr = malloc(sizeof(*playerAddr) * N); // newAddr

  socklen_t len = sizeof(masterAddr);

  char buffer[2048];
  int count = 0;

  // Print initial message
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", N);
  printf("Hops = %d\n", hops);

  // Create ringmaster socket
  master_fd = socket(AF_INET, SOCK_STREAM, 0); /* need to change AF_INET? */
  // error message
  if (master_fd < 0) {
    perror("[-]Error in connection.\n");
    exit(1);
  }

  // Get ip
  char hostname[128];
  if (gethostname(hostname, sizeof(hostname)) == -1) {
    return -1;
  }
  struct hostent *hent;
  hent = gethostbyname(hostname);
  if (NULL == hent) {
    return -1;
  }

  char *serIP;
  // get from h_addr
  serIP = inet_ntoa(*(struct in_addr *)hent->h_addr);

  // Initial memory address
  memset(&masterAddr, '\0', sizeof(masterAddr));
  masterAddr.sin_family = AF_INET;
  masterAddr.sin_port = htons(PORT);
  masterAddr.sin_addr.s_addr = inet_addr(serIP);

  // Bind socket to address:port
  ret = bind(master_fd, (struct sockaddr *)&masterAddr, sizeof(masterAddr));
  // error message
  if (ret < 0) {
    perror("[-]Error in binding.\n");
    exit(1);
  }

  // Listening
  if (listen(master_fd, N) != 0) { /* need to change N? */
    perror("[-]Error in listening.\n");
    exit(1);
  }

  while (1) {
    if (playerID < N) { // prepare game
      player_fd[playerID] =
          accept(master_fd, (struct sockaddr *)&playerAddr[playerID], &len);
      if (player_fd[playerID] < 0) {
        exit(1);
      }

      // set buffer
      memset(buffer, '\0', sizeof(buffer));
      sprintf(buffer, "%d %d ", playerID, N);

      // send ID
      send(player_fd[playerID], buffer, 2048, 0);

      // receive PORT
      memset(buffer, '\0', sizeof(buffer));
      recv(player_fd[playerID], buffer, 2048, 0);
      player_PORT[playerID] = atoi(buffer);

      // reset buffer & increase ID
      memset(buffer, '\0', sizeof(buffer));
      playerID++;
    } else { // start game
      if (count ==
          0) { // step1: send address of left neighbor - "leftAddr leftPORT "

        for (int i = 0; i < N; i++) {
          int left = i == 0 ? N - 1 : i - 1;

          // set buffer
          memset(buffer, '\0', sizeof(buffer));
          sprintf(buffer, "%s %d ", inet_ntoa(playerAddr[left].sin_addr),
                  player_PORT[left]);

          send(player_fd[i], buffer, 2048, 0);
        }

        count++;
      } else if (count == 1) { // step2: throw the potato
        // generate player
        srand(time(0));
        int startplayer = rand() % N;

        // stop for some while
        sleep(2);

        // set buffer for start
        memset(buffer, '\0', sizeof(buffer));
        sprintf(buffer, "%d", hops);

        // send to player and print message
        send(player_fd[startplayer], buffer, 2048, 0);
        printf("Ready to start the game, sending potato to player %d\n",
               startplayer);

        // select
        while (1) {
          // reset buffer
          memset(buffer, '\0', sizeof(buffer));

          FD_ZERO(&readfds);
          int max_fd = player_fd[0];
          for (int i = 0; i < N; i++) {
            FD_SET(player_fd[i], &readfds);
            if (player_fd[i] > max_fd)
              max_fd = player_fd[i];
          }
          // wait for an activity on one of the sockets , timeout is NULL , so
          // wait infinitely
          int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
          if (activity < 0) {
            printf("Select error\n");
          }

          // receive hop
          for (int i = 0; i < N; i++) {
            if (FD_ISSET(player_fd[i], &readfds)) {
              if (recv(player_fd[i], buffer, 2048, 0) < 0) {
                perror("Error in receiving hop\n");
                exit(1);
              }

              printf("Trace of potato:\n");

              char temp[2048] = {'\0'};
              strcpy(temp, buffer);

              char *curt = strtok(temp, " ");
              while (curt != NULL) {
                curt = strtok(NULL, " ");
                if (curt == NULL)
                  break;
                int num = atoi(curt);
                printf("<%d>", num);
                if (hops > 1)
                  printf(",");
                else
                  printf("\n");
                hops--;
              }

              sleep(1);

              // send everyone back
              for (int j = 0; j < N; j++) {
                // set buffer to -1
                memset(buffer, '\0', sizeof(buffer));
                int end = -1;
                sprintf(buffer, "%d ", end);

                send(player_fd[j], buffer, 2048, 0);
              }

              sleep(1);

              // close the game
              close(master_fd);
              for (int i = 0; i < N; i++) {
                close(player_fd[i]);
              }
              free(playerAddr);
              free(player_PORT);
              free(player_fd);

              exit(1);
            }
          }
        }
      }
    }
  }

  return 0;
}
