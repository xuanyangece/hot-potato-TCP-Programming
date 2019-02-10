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
  if (argc != 4) {
    perror("Program invoke format: ringmaster <port_num> <num_players> <num_hops>\n");
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
  int master_fd = 0;
  int *player_fd = malloc(sizeof(*player_fd) * N);
  struct sockaddr_in masterAddr;
  struct sockaddr_in *playerAddr = malloc(sizeof(*playerAddr) * N);
  int playerID = 0;
  pid_t pid;

  char buffer[512];
  socklen_t len;

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

  // Initial memory address
  memset(&masterAddr, 0, sizeof(masterAddr));
  masterAddr.sin_family = AF_INET;
  masterAddr.sin_port = htons(PORT);
  masterAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  /* need to change addr */

  // Bind socket to address:port
  if (bind(master_fd, (struct sockaddr*)&masterAddr, sizeof(masterAddr)) < 0) {
    perror("Error in binding.\n");
    exit(1);
  }

  // Listening
  if (listen(master_fd, N) != 0) {
    perror("Error in listening.\n");
    exit(1);
  }

  while (1) {
    if (playerID < N) { // prepare game
      player_fd[playerID] = accept(master_fd, (struct sockaddr*)&playerAddr[playerID], &len);
      if (player_fd[playerID] < 0) exit(1);
      printf("Player %d is ready to play\n", playerID);

      if ((pid = fork()) == 0) { // child
	close(master_fd);

	// set buffer
	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%d %d ", playerID, N);

	// send
	send(player_fd[playerID], buffer, 512, 0);
      }
      else { // parent
	playerID++;
      }
    }
    else { // start the game
      printf("Ready to start the game, sending potato to player X\n");
      break;
    }
  }

  // close & free
  close(master_fd);
  for (int i = 0; i < N; i++) {
      close(player_fd[i]);
  }
  free(playerAddr);
  free(player_fd);
  
  return 0;
}
