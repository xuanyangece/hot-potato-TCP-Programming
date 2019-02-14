#include <arpa/inet.h>
#include <netdb.h>
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
  char buffer[2048];

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
  int PORTL = 1025;
  struct sockaddr_in leftAddr; // for sending
  int right_r_fd;
  int right_s_fd;
  int PORTR = 1026;
  struct sockaddr_in rightAddr; // for sending
  fd_set readfds;
  socklen_t len = sizeof(selfAddr);

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

  printf("My IP is: %s\n", selfIP);

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

  while (1) {
    if (count == 0) {
      printf("ready to receive\n");
      if (recv(player_fd, buffer, 2048, 0) <
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
        printf("Connected as player %d out of %d total players\n", ID + 1, N);

        // set buffer
        memset(buffer, '\0', sizeof(buffer));
        sprintf(buffer, "%d", PORTR);

        // send port number
        send(player_fd, buffer, 2048, 0);

        // reset buffer & update count
        memset(buffer, '\0', sizeof(buffer));
        count++;
      }
    } else if (count == 1) {
      if (recv(player_fd, buffer, 2048, 0) <
          0) { // second recv: "leftIP leftPORT " - get address for neighbor and
               // build connection
        printf("Error in receiving data 1.\n");
      } else {
        /***************************************************
         * left socket sends connection, right socket waits *
         ***************************************************/

        if (ID == 0) { // for the first one, accept first
          // accept
          right_s_fd = accept(right_r_fd, (struct sockaddr *)&selfAddr, &len);

          if (right_s_fd < 0) {
            perror("right accept fails\n");
            exit(1);
          }

          printf("ID %d right port successfully accept\n", ID);
        }

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

        if (ID != 0) { // for the rest, coonect first, accept later
          // accept
          right_s_fd = accept(right_r_fd, (struct sockaddr *)&selfAddr, &len);

          if (right_s_fd < 0) {
            perror("right accept fails\n");
            exit(1);
          }

          printf("ID %d right port successfully accept\n", ID);
        }

        // reset buffer & update count
        memset(buffer, '\0', sizeof(buffer));
        count++;
      }
    } else if (count == 2) {
      printf("\nAll connection established! Let's get some potato!\n");

      int activity = 0;
      int max_fd = 1;
      if (left_fd > right_s_fd && left_fd > player_fd)
        max_fd = left_fd;
      else if (right_s_fd > left_fd && right_s_fd > player_fd)
        max_fd = right_s_fd;
      else
        max_fd = player_fd;

      /**************************
       *     fd_set & select     *
       **************************/
      while (1) {
        FD_ZERO(&readfds);
        // socket to read set
        FD_SET(left_fd, &readfds);
        FD_SET(right_s_fd, &readfds);
        FD_SET(player_fd, &readfds);

        printf("Just before select\n");
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
          printf("Select error\n");
        }
        printf("Just after select\n");

        // reset buffer
        memset(buffer, '\0', sizeof(buffer));

        // receive hop
        int res = 0;
        if (FD_ISSET(left_fd, &readfds))
          res = recv(left_fd, buffer, 2048, 0);
        else if (FD_ISSET(right_s_fd, &readfds))
          res = recv(right_s_fd, buffer, 2048, 0);
        else
          res = recv(player_fd, buffer, 2048, 0);
        if (res < 0) {
          perror("Error in receiving hop\n");
          exit(1);
        }

        printf("Copy buffer: '%s'\n", buffer);
        // parse buffer
        int ending = 0;
        char temp[2048] = {'\0'};
        strcpy(temp, buffer);

        // count buffer
        printf("Count buffer\n");
        if (temp[0] == '\0')
          break;
        char *curt = strtok(temp, " ");
        printf("Strtok buffer");
        int count = atoi(curt);
        printf("count: %d\n", count);

        if (count >= 0) { // not end
          // count how many iterations have been made
          while (curt != NULL) {
            count--;
            curt = strtok(NULL, " ");
          }

          // right on target, stop
          if (count == 0) {
            // get added string
            char tempadd[20] = {'\0'};
            memset(tempadd, '\0', sizeof(tempadd));
            sprintf(tempadd, " %d", ID);

            // concatenate
            strcat(buffer, tempadd);

            send(player_fd, buffer, 2048, 0);
            printf("I'm it\n");
          } else { // continue
            // get added string
            char tempadd[20] = {'\0'};
            memset(tempadd, '\0', sizeof(tempadd));
            sprintf(tempadd, " %d", ID);

            // concatenate
            strcat(buffer, tempadd);

            // choose neighbor
            srand(time(0));
            int option = (rand() % 10) + 1;

            sleep(1);

            // send to neighbor
            if (option <= 5) { // send to left
              send(left_fd, buffer, 2048, 0);
              int receiver = ID == 0 ? N - 1 : ID - 1;
              printf("Sending potato to %d\n", receiver);
            } else { // send to right
              send(right_s_fd, buffer, 2048, 0);
              int receiver = ID == N - 1 ? 0 : ID + 1;
              printf("Sending potato to %d\n", receiver);
            }

            printf("I'm sending %s\n", buffer);

            // reset buffer
            memset(buffer, '\0', sizeof(buffer));
          }
        } else { // end
          break;
        }
      }

      // TEMPORT CLOSE
      close(player_fd);
      close(left_fd);
      close(right_r_fd);
      close(right_s_fd);
      printf("Disconnect, ready to exit\n");
      exit(1);
    }
  }

  return 0;
}
