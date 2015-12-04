#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <signal.h>/* signal name macros, and the kill() prototype */
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define PCKT_SIZE 1000
#define MAX_PAYLOAD 994
#define HEADER_SIZE 6

//Define TYPE of packet
#define REQ 0      //initial request
#define DATA 1
#define ACK 2
#define FIN 3

const char* NO_FILE_MSG ="Error: File does not exist.";

void error(char *msg)
{
  perror(msg);
  exit(1);
}

//Stores header information of packet into 2 bytes
//at the beginning of each character array packet
typedef struct header {
  unsigned int type:2;
  unsigned int seq_num:28; //sequence number of packet sent
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

  if (argc != 5) 
    error("ERROR Usage: ./server <port_number> CWnd packet_loss packet_corruption");
  
  //change this into an arg later
  int CWnd = atoi(argv[2]);
  int pckt_loss = atoi(argv[3]);
  int pckt_corr = atoi(argv[4]);
  //printf("Packet corruption percentage: %i\n", pckt_corr);
  int CWnd_thresh = 1;
  
  memset(buffer, 0, PCKT_SIZE); //reset memory
  memset(filename, 0, MAX_PAYLOAD);
  
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
  header_t* rcv_header = (header_t*) (&buffer);
  char* payload = buffer + HEADER_SIZE;
  strcpy(filename, payload);

  //create packet for transmission
  char packet[PCKT_SIZE];
  memset(packet, 0, PCKT_SIZE);
 
  header_t* header = (header_t*) (&packet);
  payload = packet + HEADER_SIZE;
  
  //get file
  int file_fd;
  file_fd = open(filename, O_RDONLY);

  if (file_fd < 0) {
    //send message to client
    header->type = FIN;
    header->seq_num = 0;
    header->size = strlen(NO_FILE_MSG);
    strcpy(payload, NO_FILE_MSG);

    n = sendto(sockfd, packet, (header->size) + HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, clilen);

    //throw error
    error("Error: File does not exist.");
  }
  //find size of the file
  int beginning = lseek(file_fd, 0, SEEK_CUR);
  int file_length = lseek(file_fd, 0, SEEK_END);
  lseek(file_fd, beginning, SEEK_SET);
  //store file contents into temp buffer
  //char file_data[file_length];
  char* file_data = malloc(file_length);
  memset(file_data, 0, file_length);
  n = read(file_fd, file_data, file_length);

  
  
  int packets_in_flight = 0;
  int data_count = 0;
  int dup_ACK = 0;
  int is_dup = 0;
  int timeout_occurred = 0;
  int latest_ACK = 0;
  int old_rcv = 0;
  srand(time(0));
  int r_l;
  int r_c;

  //timeout variables
  int result;
  fd_set rset;
  struct timeval timeout;
  
  //send packets until last ACK is received
  while (latest_ACK != file_length) {

    printf("\n");
    
    r_l = rand() % 100;
    r_c = rand() % 100;
    
    //once all packets in window sent, check for ACK
    if (packets_in_flight == CWnd || data_count == file_length) {
      memset(buffer, 0, PCKT_SIZE);
      printf("Waiting for an ACK.\n");
      
      //timeout select
      timeout.tv_sec = (double) 1;
      timeout.tv_usec = 0;
      FD_ZERO(&rset);
      FD_SET(sockfd, &rset);
      result = select(sockfd + 1, &rset, NULL, NULL, &timeout);

      if (result < 0) {
	error("ERROR on select.");
      }
      else if (result == 0) {
	timeout_occurred = 1;
	//printf("Select returned 0.\n");
      }
      else if (FD_ISSET(sockfd, &rset)) {
	recvlen = recvfrom(sockfd, buffer, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
	if (recvlen == 0)
	  break;
      }
      else
	break;

      //Packet loss and corruption, we simply ignore the packet.
      if (!timeout_occurred) {
	if (pckt_corr >= r_c && pckt_corr != 0) {
	  printf("Packet corruption.\n");
	  continue;
	}
	else if (pckt_loss >= r_l && pckt_loss != 0) {
	  printf("Packet loss.\n");
	  continue;
	}
      }

      rcv_header = (header_t*) &buffer;

      //Checking and incrementing duplicate ACKs
      if (rcv_header->seq_num == old_rcv) {
	dup_ACK++;
	is_dup = 1;
	printf("Received duplicate ACK %i.\n", dup_ACK);
      }
      if (dup_ACK == 3 || timeout_occurred) {
	//go-back-n retransmission
	packets_in_flight = 0;
	data_count = old_rcv;
	//printf("Inside retransmission, data count is now %i.\n", data_count);
	if (timeout_occurred)
	  printf("Timeout occurred.\n");
	if (dup_ACK == 3) 
	  printf("Duplicate ACKs received. Retransmitting window from byte number %i\n", data_count);
	dup_ACK = 0;
	timeout_occurred = 0; 
      }
      else if (rcv_header->seq_num != old_rcv) {
	printf("ACK %i was received!\n", rcv_header->seq_num);
	latest_ACK = rcv_header->seq_num;
	old_rcv = rcv_header->seq_num;
	if (packets_in_flight > 0)
	  packets_in_flight--;
      }

      //Continue to next iteration of loop if duplicate already detected.
      if (is_dup) {
	is_dup = 0;
	continue;
      }
    }

    if (data_count == file_length)
      continue;
    
    memset(packet, 0, PCKT_SIZE);
    
    //fill in header information
    header->type = DATA;
    header->seq_num = data_count;

    int i;
    //read contents of file into payload until EOF or MAX_PAYLOAD
    for (i = 0; i != MAX_PAYLOAD; i++) {
      //reached EOF
      if (data_count == file_length){
     	break;
      }
      payload[i] = file_data[data_count];
      data_count++;
      //printf("Data count is now %i\n", data_count);
    }
    if (data_count == file_length){
	header->type = FIN;
	printf("Sending a FIN\n");
    }
    header->size = i;

    //printf("This packet contains the following: %s\n", payload);
    
    //send packet to client based on payload size + 4 bytes of header
    n = sendto(sockfd, packet, header->size + HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, clilen);
    if (n < 0)
      error("ERROR on packet send.");

    //printf("There are %i bytes in the payload.\n", header->size);
    
    printf("Sent packet sequence number %i.\n", header->seq_num);
    packets_in_flight++;
  }

  //Send final packet
  header->type = ACK;
  header->seq_num = 0;
  header->size = 0;
  memset(payload, 0, MAX_PAYLOAD); 

  while (1) { 
    n = sendto(sockfd, packet, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, clilen);

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    result = select(sockfd + 1, &rset, NULL, NULL, &timeout);

    if (result < 0) {
      error("ERROR on select.");
    }
    else if (result == 0) {
      break;
    }
    else if (FD_ISSET(sockfd, &rset)) {
      //In case client didn't get last ACK, is sending more requests
      recvlen = recvfrom(sockfd, buffer, HEADER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
    }
  }
  
  //close connection
  free(file_data);
  close(sockfd);
  
  return 0;
}
