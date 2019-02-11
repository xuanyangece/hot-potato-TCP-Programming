#include <unistd.h>
#include <netdb.h>  //gethostbyname
#include <arpa/inet.h>  //ntohl
#include <stdio.h>

int main() {
    char hostname[128];
    int ret = gethostname(hostname, sizeof(hostname));
    if (ret == -1){
        return -1;
    }
    struct hostent *hent;
    hent = gethostbyname(hostname);
    if (NULL == hent) {
        return -1;
    }

    char * iip;
    //直接取h_addr_list列表中的第一个地址h_addr
    iip = inet_ntoa(*(struct in_addr*)hent->h_addr);
    printf("%s", iip);
    return 0;
}

