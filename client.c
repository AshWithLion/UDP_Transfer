#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PCKT_SIZE 1000
#define MAX_PAYLOAD 996

//Booleans
#define true 1
#define false 0

//Define TYPE of packet
#define REQ 0      //initial request
#define DATA 1
#define ACK 2
#define FIN 3

//Stores header information of packet into 2 bytes
//at the beginning of each character array packet
typedef struct header {
  unsigned int type:2;
  unsigned int seq_num:20; //sequence number of packet sent
  unsigned int size:10;    //size of payload
} header_t;

void error(char *message) {
  perror(message);
  exit(0);
}

int main(int argc, char* argv[]) {
  int socketfd, port_number, current_position;
  char *host_name, *file_name;
  double loss, corruption;
  struct sockaddr_in server_address;
  struct hostent *server;
  socklen_t server_length;
  char buffer[PCKT_SIZE];

  
  host_name = argv[1];
  port_number = atoi(argv[2]);
  file_name = argv[3];

  // socket open
  if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      error("Fail to open socket\n");
    }

  // set server information
  if((server = gethostbyname(host_name)) == NULL)
    {
      error("ERROR no such host\n");
      exit(1);
    }

  server_length = sizeof(server_address);
  //set space for server_address
  memset((char *) &server_address, 0, server_length);
  //IPv4
  server_address.sin_family = AF_INET;

  memcpy((char *) &server_address.sin_addr.s_addr,(char *) server->h_addr, server->h_length);
  server_address.sin_port = htons(port_number);

  header_t* head = (header_t*) &buffer;
  char* payload = buffer + 4;

  head->type = REQ;
  head->seq_num = 0;
  head->size = sizeof(payload);
  strcpy(payload, file_name);

  
  
  // send the request
  if (sendto(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &server_address, server_length) < 0)
    {
      error("ERROR sending request\n");

    }

  close(socketfd);
}
