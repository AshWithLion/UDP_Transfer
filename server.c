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

#define PCKT_SIZE 1000
#define MAX_PAYLOAD 3
#define TIMEOUT 15000

//Booleans
#define true 1
#define false 0

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
  char filename[20];

  //change this into an arg later
  int CWnd = atoi(argv[2]);
  int pckt_corr = atoi(argv[3]);
  int pckt_loss = atoi(argv[4]);
  printf("Packet corruption percentage: %i\n", pckt_corr);
  int CWnd_thresh = 1;

  //timeout variables
  struct timespec start;
  struct timespec end;

  memset(buffer, 0, PCKT_SIZE); //reset memory
  memset(filename, 0, 20);

  if (argc < 2) {
    fprintf(stderr,"ERROR, missing port number\n");
    exit(1);
  }
  
  //create a UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM | O_NONBLOCK, 0);
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
  do {
  recvlen = recvfrom(sockfd, buffer, PCKT_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
  } while (recvlen < 0);
  
  //extract header + payload info from buffer
  header_t* rcv_header = (header_t*) (&buffer);
  char* payload = buffer + 4;
    
  strcpy(filename, payload);
  
  //create packet for transmission
  char packet[PCKT_SIZE];
  memset(packet, 0, PCKT_SIZE);

  header_t* header = (header_t*) (&packet);
  payload = packet + 4;

  //get file
  int file_fd;
  file_fd = open(filename, O_RDONLY);
  if (file_fd < 0) {
    //send message to client
    header->type = FIN;
    header->seq_num = 1;
    header->size = strlen(NO_FILE_MSG);
    strcpy(payload, NO_FILE_MSG);

    n = sendto(sockfd, packet, (header->size) + 2, 0, (struct sockaddr *) &cli_addr, clilen);
    

    //throw error
    error("Error: File does not exist.");
  }
  
  //find size of the file
  int beginning = lseek(file_fd, 0, SEEK_CUR);
  int file_length = lseek(file_fd, 0, SEEK_END);
  lseek(file_fd, beginning, SEEK_SET);

  //store file contents into temp buffer
  char file_data[file_length];
  memset(file_data, 0, file_length);
  n = read(file_fd, file_data, file_length);

  printf("%s\n%i\n", file_data, file_length);
  
  
  int packets_in_flight = 0;
  int data_count = 0;
  int dup_ACK = 0;
  int timeout_occurred = 0;
  int first_packet = 1;
  srand(time(0));
  int r;
  
  printf("%i\n", rcv_header->seq_num);
  
  //send packets until last ACK is received
  while (data_count != file_length) {

    int old_rcv = rcv_header->seq_num;
    r = rand() % 100;
    
    //once all packets in window sent, check for ACK
    if (packets_in_flight == CWnd) {

      memset(buffer, 0, PCKT_SIZE);
      printf("Doing some flow control. Waiting for an ACK.\n");
      

      do {
	//waiting for an ACK
	n = recvfrom(sockfd, buffer, 2, 0, (struct sockaddr *) &cli_addr, &clilen);

	//check for timeout
	clock_gettime(CLOCK_MONOTONIC, &end);
	time_t difference = end.tv_nsec - start.tv_nsec;
	if (difference > TIMEOUT) {
	  printf("Oops, timeout occurred after %d nanoseconds\n", difference);
	  timeout_occurred = 1;
	  break;
	}
      }
      while (n < 0);
      
      //Packet loss and corruption, we simply ignore the packet.
      if (pckt_corr >= r) {
	printf("Packet corruption.\n");
	       continue;
      }
      else if (pckt_loss >= r) {
	printf("Packet loss.\n");
	continue;
      }

      rcv_header = (header_t*) &buffer;

      //Checking and incrementing duplicate ACKs
      if (rcv_header->seq_num == old_rcv) {
	dup_ACK++;
      }

      if (dup_ACK == 3 || timeout_occurred == 1) {
	//go-back-n retransmission
	packets_in_flight = 0;
	data_count = rcv_header->seq_num;
	printf("Duplicate ACKs received. Retransmitting window from byte number %i\n", data_count);
	dup_ACK = 0;
	timeout_occurred = 0;
	clock_gettime(CLOCK_MONOTONIC, &start);
      }
      else if (rcv_header->seq_num != old_rcv) {
	rcv_header->seq_num = data_count;
	printf("ACK %i was received!\n", (rcv_header->seq_num) + 1);
	CWnd_thresh += 1;
	packets_in_flight--;
      }
    }

    memset(packet, 0, PCKT_SIZE);

    //fill in header information
    header->type = DATA;
    header->seq_num = data_count;

    int i;
    //read contents of file into payload until EOF or MAX_PAYLOAD
    for (i = 0; i != MAX_PAYLOAD; i++) {
      //reached EOF
      if (data_count == file_length){
	header->type = FIN;
	break;
      }
      payload[i] = file_data[data_count];
      data_count++;
      printf("Data count is now %i\n", data_count);
    }
    header->size = i;

    printf("This packet contains the following: %s\n", payload);
    
    //send packet to client based on payload size + 2 bytes of header
    n = sendto(sockfd, packet, (header->size) + 2, 0, (struct sockaddr *) &cli_addr, clilen);
    if (n < 0)
      error("ERROR on packet send.");

    if (first_packet == 1) {
      clock_gettime(CLOCK_MONOTONIC, &start);
      first_packet = 0;
    }

    printf("Sent packet sequence number %i\n", header->seq_num);
    packets_in_flight++;
    
  }
  
  //close connection
  close(sockfd);
  
  return 0;
}
