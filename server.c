#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <signal.h>/* signal name macros, and the kill() prototype */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

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


void error(char *msg)
{
  perror(msg);
  exit(1);
}

//Stores header information of packet into 2 bytes
//at the beginning of each character array packet
typedef struct header {
  unsigned int type:2;
  unsigned int seq_num:20; //sequence number of packet sent
  unsigned int size:10;    //size of payload
} header_t;

int main(int argc, char *argv[])
{
  int sockfd, portno, recvlen;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  char buffer[PCKT_SIZE];
  char filename[MAX_PAYLOAD];
  
  memset(buffer, 0, PCKT_SIZE); //reset memory

  if (argc < 2) {
    fprintf(stderr,"ERROR, missing port number\n");
    exit(1);
  }

  //create a UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  //reset memory, fill in address info
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  //bind socket to IP address and port
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");

  clilen = sizeof(cli_addr);

  //receive initial request from client
  recvlen = recvfrom(sockfd, buffer, PCKT_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

  //extract header + payload info from buffer
  header_t* header = (header_t*) (&buffer);
  char* payload = buffer + 4;

  int i;
  for (i = 0; i != header->size; i++)
    filename[i] = payload[i];    //get filename
  
  //get file
  int file_fd;
  file_fd = open(filename, O_RDONLY);
  if (file_fd < 0)
    error("Error: File does not exist.");
    //send message to client
    //n = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, clilen);

  //find size of the file
  int beginning = lseek(file_fd, 0, SEEK_CUR);
  int file_length = lseek(file_fd, 0, SEEK_END);
  lseek(file_fd, beginning, SEEK_SET);

  int packet_count = 1;
  int data_count = 0;

  //send packets until no more data is left in the file
  while (data_count != file_length) {  

    char packet[PCKT_SIZE];
    memset(packet, 0, PCKT_SIZE);
  
    payload = packet + 4;
    header = (header_t*) (&packet);

    //fill in header information
    header->type = DATA;
    header->seq_num = packet_count;

    //read contents of file into payload until EOF or MAX_PAYLOAD
    for (i = 0; i != MAX_PAYLOAD; i++) {
      n = read(file_fd, payload + i, 1);
      if (n < 0)
	error("ERROR reading file into payload buffer.");
      //reached EOF
      if (n == 0){
	header->type = FIN;
	break;
      }
      data_count++;
    }
    
    header->size = i;

    //send packet to client based on payload size + 2 bytes of header
    n = sendto(sockfd, packet, header->size + 2, 0, (struct sockaddr *) &cli_addr, clilen);
    if (n < 0)
      error("ERROR on packet send.");

    //increase seq_no count
    packet_count++;
  }
  
  //close connection
  close(sockfd);

  return 0;
}

