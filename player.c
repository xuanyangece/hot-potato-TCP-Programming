#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {
  if (argc != 3) {
    perror("Program invoke format: player <machine_name> <port_num>\n");
    exit(1);
  }

  // Get master ip
  struct hostent *hent;
  hent = gethostbyname(argv[1]);
  if (NULL == hent) {
    return -1;
  }
  char *serIP;
  serIP = inet_ntoa(*(struct in_addr *)hent->h_addr);

  // master parameter
  struct sockaddr_in masterAddr;
  int PORT = atoi(argv[2]);

  // player parameters
  int count = 0;
  int player_fd;
  int ID;
  int N;
  char buffer[512];

  /***********************
   *        master        *
   ***********************/
  // Get master address
  memset(&masterAddr, 0, sizeof(masterAddr));
  masterAddr.sin_family = AF_INET;
  masterAddr.sin_port = htons(PORT);
  masterAddr.sin_addr.s_addr = inet_addr(serIP);

  // create socket - self <-> ringmaster
  player_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (player_fd == -1) {
    perror("Error: cannot create socket\n");
    exit(1);
  } // if

  // connect
  if (connect(player_fd, (struct sockaddr *)&masterAddr, sizeof(masterAddr)) <
      0) {
    perror("Error: cannot connect to socket\n");
    exit(1);
  }

  // neighbor parameters
  struct sockaddr_in selfAddr;
  int left_fd;
  int PORTL = 11;
  struct sockaddr_in leftAddr; // for sending
  int right_r_fd;
  int right_s_fd;
  int PORTR = 13;
  struct sockaddr_in rightAddr; // for sending
  socklen_t len;

  /***********************
   *         left         *
   ***********************/
  left_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (left_fd == -1) {
    perror("Error: cannot create socket\n");
    exit(1);
  } // if

  /************************
   *        right         *
   ***********************/
  right_r_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (right_r_fd == -1) {
    perror("Error: cannot create socket\n");
    exit(1);
  } // if

  // Get self ip
  char selfname[128];
  if (gethostname(selfname, sizeof(selfname)) == -1) {
    return -1;
  }
  struct hostent *selfhent;
  selfhent = gethostbyname(selfname);
  if (selfhent == NULL) {
    return -1;
  }
  char *selfIP;
  // get from h_addr
  selfIP = inet_ntoa(*(struct in_addr *)selfhent->h_addr);

  // Initial memory address
  memset(&selfAddr, '\0', sizeof(selfAddr));
  selfAddr.sin_family = AF_INET;
  selfAddr.sin_port = htons(PORTR);
  selfAddr.sin_addr.s_addr = inet_addr(selfIP);

  // bind while cannot succeed
  while (bind(right_r_fd, (struct sockaddr *)&selfAddr, sizeof(selfAddr)) < 0) {
    PORTR = PORTR == 65535 ? 0 : PORTR + 1;
    selfAddr.sin_port = htons(PORTR);
    selfAddr.sin_family = AF_INET;
    selfAddr.sin_addr.s_addr = inet_addr(selfIP);
  }

  printf("Successfully bind port: %d\n", PORTR);

  // Listening
  if (listen(right_r_fd, 1) != 0) { /* need to change N? */
    perror("Error in listening right.\n");
    exit(1);
  }

  printf("Successfully listening\n");

  // accept
  right_s_fd = accept(right_r_fd, (struct sockaddr *)&selfAddr, &len);
  if (right_s_fd < 0) {
    perror("right accept fails\n");
    exit(1);
  }

  printf("Successfully accept\n");

  while (1) {
    if (count == 0) {
      printf("ready to receive\n");
      if (recv(player_fd, buffer, 512, 0) <
          0) // first recv: "ID N " - used for get ID and N
        printf("Error in receiving data 0.\n");
      else {
        /*****************************************
         *     receive info & send port number    *
         *****************************************/
        // get info
        char *id = strtok(buffer, " ");
        char *n = strtok(NULL, " ");
        ID = atoi(id);
        N = atoi(n);

        // print status
        printf("Connected as player %s out of %s total players\n", id, n);

        // set buffer
        memset(buffer, '\0', sizeof(buffer));
        sprintf(buffer, "%d", PORTR);

        // send port number
        send(player_fd, buffer, 512, 0);

        // reset buffer & update count
        memset(buffer, '\0', sizeof(buffer));
        count++;
      }
    } else if (count == 1) {
      if (recv(player_fd, buffer, 512, 0) <
          0) { // second recv: "leftIP leftPORT " - get address for neighbor and
               // build connection
        printf("Error in receiving data 1.\n");
      } else {
        /***************************************************
         * left socket sends connection, right socket waits *
         ***************************************************/

        char *leftIP = strtok(buffer, " ");
        char *leftPORT = strtok(NULL, " ");
        PORTL = atoi(leftPORT);

        // step1: get IP from buffer
        memset(&leftAddr, 0, sizeof(leftAddr));
        leftAddr.sin_family = AF_INET;
        leftAddr.sin_port = htons(PORTL);
        leftAddr.sin_addr.s_addr = inet_addr(leftIP);

        // step2: connect
        if (connect(left_fd, (struct sockaddr *)&leftAddr, sizeof(leftAddr)) <
            0) {
          perror("Error: cannot connect to left socket\n");
          exit(1);
        }

        printf("Connect player %d's left socket to %s\n", ID, leftIP);

        // reset buffer & update count
        memset(buffer, '\0', sizeof(buffer));
        count++;
      }
    } else if (count == 2) {
      if (recv(player_fd, buffer, 512, 0) <
          0) { // third recv: "hops " - get hops from server
        printf("Error in receiving data 2.\n");
      } else {
      }
    }

    // close(player_fd);
    // printf("Disconnect\n");
    // exit(1);
  }
  return 0;
}
