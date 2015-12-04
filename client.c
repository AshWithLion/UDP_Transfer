
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define PCKT_SIZE 1000
#define MAX_PAYLOAD 994
#define HEADER_SIZE 6

//Define TYPE of packet
#define REQ 0      //initial request
#define DATA 1
#define ACK 2
#define FIN 3

//Stores header information of packet into 2 bytes
//at the beginning of each character array packet
typedef struct header {
  unsigned int type:2;
  unsigned int seq_num:28; //sequence number of packet sent
  unsigned int size:10;    //size of payload
} header_t;

void error(char *message) {
  perror(message);
  exit(0);
}

int main(int argc, char* argv[]) {
  int socketfd, port_number, current_position;
  char *host_name, *file_name;
  int loss, corruption;
  struct sockaddr_in server_address;
  struct hostent *server;
  socklen_t server_length;
  char buffer[PCKT_SIZE];
  int file_fd;
  int recv = 0;
  int n = 0;
  
  host_name = argv[1];
  port_number = atoi(argv[2]);
  file_name = argv[3];
  loss = atoi(argv[4]);
  corruption = atoi(argv[5]);

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
  char* payload = buffer + HEADER_SIZE;

  head->type = REQ;
  head->seq_num = 0;
  strcpy(payload, file_name);
  head->size = strlen(payload);
  
  // send the request
  if (sendto(socketfd, buffer, head->size + HEADER_SIZE, 0, (struct sockaddr*) &server_address, server_length) < 0)
      error("ERROR sending request\n");

  char * file_name_copy = strcat(file_name, "_copy");
  file_fd = open(file_name_copy, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  if (file_fd < 0)
    error("Error creating file to read into.");

  char* ACK_packet[HEADER_SIZE];
  header_t* ACK_head = (header_t*) &ACK_packet;
  int ACK_number = 0;
  srand(time(0));

  ACK_head->type = ACK;
  ACK_head->seq_num = 0;
  ACK_head->size = 0;
  
  //receive data packets
  while (1) {

    int r = rand() % 100;
    
    memset(buffer, 0, PCKT_SIZE);

    recv = recvfrom(socketfd, buffer, PCKT_SIZE, 0, (struct sockaddr *) &server_address, &server_length);

    printf("Received packet with sequence number %i.\n", head->seq_num);
    
    if (loss >= r) {
      printf("Packet with sequence number %i lost.\n", head->seq_num);
      continue;
    }
    else if (corruption >= r) 
      printf("Packet with sequence number %i corrupted.\n", head->seq_num);
    else if (head->seq_num == ACK_number) {
      printf("Received a packet!\n");  
      n = write(file_fd, payload, head->size);

      ACK_number = head->seq_num + head->size;
      printf("Sending ACK %i.\n", ACK_number);
    
      //send an ACK
      ACK_head->seq_num = ACK_number;

      if (head->type == FIN) {
	printf("FIN packet has sequence number %i.\n", ACK_head->seq_num);
	break;
      }
    }
    else {
      //Ignore all out of sequence packets
      printf("Duplicate ACK. Still expecting packet %i.\n", ACK_head->seq_num);
    }
    
    n = sendto(socketfd, ACK_packet, HEADER_SIZE, 0, (struct sockaddr *) &server_address, server_length);

    printf("\n");
  }

  do {
    n = sendto(socketfd, ACK_packet, HEADER_SIZE, 0, (struct sockaddr *) &server_address, server_length);
    recv = recvfrom(socketfd, buffer, HEADER_SIZE, 0, (struct sockaddr *) &server_address, &server_length);
  } while (head->type != ACK);
 
  close(socketfd);
}
