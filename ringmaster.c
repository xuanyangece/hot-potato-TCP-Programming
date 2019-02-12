#include <arpa/inet.h>
#include <netdb.h> // gethostbyname
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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

  if (N < 1 || hops < 0 || hops > 512) {
    perror("Invalid num_players or num_hops\n");
    exit(1);
  }

  // socket variables
  int master_fd, ret;
  struct sockaddr_in masterAddr;

  int playerID = 0;
  int *player_fd = malloc(sizeof(*player_fd) * N); // newSocket
  int *player_PORT = malloc(sizeof(*player_PORT) * N);
  struct sockaddr_in *playerAddr = malloc(sizeof(*playerAddr) * N); // newAddr

  socklen_t len = sizeof(masterAddr);

  char buffer[512];
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
      printf("Player %d is ready to play\n", playerID);
      printf("Accept from %s\n", inet_ntoa(playerAddr[playerID].sin_addr));

      // set buffer
      memset(buffer, '\0', sizeof(buffer));
      sprintf(buffer, "%d %d ", playerID, N);

      // send ID
      send(player_fd[playerID], buffer, 512, 0);

      // receive PORT
      memset(buffer, '\0', sizeof(buffer));
      recv(player_fd[playerID], buffer, 512, 0);
      player_PORT[playerID] = atoi(buffer);

      // reset buffer & increase ID
      memset(buffer, '\0', sizeof(buffer));
      playerID++;
    } else { // start game
      if (count ==
          0) { // step1: send address of left neighbor - "leftAddr leftPORT "
        printf("Ready to start the game, set's get some address done\n");

        for (int i = 0; i < N; i++) {
          int left = i == 0 ? N - 1 : i - 1;

          // set buffer
          memset(buffer, '\0', sizeof(buffer));
          sprintf(buffer, "%s %d ", inet_ntoa(playerAddr[left].sin_addr),
                  player_PORT[left]);

          printf("Sending No.%d player the address of its left player No.%d: "
                 "%s - PORT: %d\n",
                 i, left, inet_ntoa(playerAddr[left].sin_addr),
                 player_PORT[left]);
          send(player_fd[i], buffer, 512, 0);
        }

        count++;
      } else if (count == 1) { // step2: throw the potato
        printf("I'm going to throw potato now\n");

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

  close(master_fd);
  for (int i = 0; i < N; i++) {
    close(player_fd[i]);
  }
  free(playerAddr);
  free(player_PORT);
  free(player_fd);

  return 0;
}
