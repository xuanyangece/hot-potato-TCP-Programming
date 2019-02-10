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
  
  int N = atoi(argv[2]);
  int hobs = atoi(argv[3]);

  if (N < 1 || hobs < 0 || hobs > 512) {
    perror("Invalid num_players or num_hops\n");
    exit(1);
  }

  return 0;
}
