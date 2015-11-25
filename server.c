#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <signal.h>/* signal name macros, and the kill() prototype */
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 2048
#define MAXFILENAMESIZE 256
#define PCKT_SIZE 1000
#define true 1
#define false 0


void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, portno, recvlen;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  char buffer[BUFSIZE];
  char filename[MAXFILENAMESIZE];
  char packet[PCKT_SIZE];
  int source, destination;
  
  memset(buffer, 0, BUFSIZE); //reset memory
  memset(packet, 0, PCKT_SIZE);

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

  //receive client message, store into buffer
  recvlen = recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
  if (recvlen < 0)
    error("ERROR receiving request");

  //read client's message, which should just be the filename
  strncpy(filename, buffer, recvlen + 1);
  //filename[recvlen + 1] = '\0';   //depends on sizeof or strlen
  printf("%s\n", buffer);
  printf("%s\n", filename);

  //create packet header - prelim source, dest. port + length of header + content
  /*source = serv_addr.sin_port;
    destination = cli_addr.sin_port; */
  
  //get file
  int file_fd;
  file_fd = open(filename, O_RDONLY);
  if (file_fd < 0) {
    strcpy(packet, "Error: File does not exist.");
    n = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, clilen);
    if (n < 0)
      error("ERROR sending message");
  }

  //Find size of the file
  /* int beginning = lseek(file_fd, 0, SEEK_CUR);
  int file_length = lseek(file_fd, 0, SEEK_END);
  lseek(file_fd, beginning, SEEK_SET);

  n = read(file_fd, packet, file_length);

  printf("%s", packet);
  
  */
  
  int data = false;
  
  //send multiple packets
  while (data == false) {

    printf("Sending a new packet.\n");
  
    int count = 0;
    int i = 0;
  
    //read data to packet buffer
    for (i = 0; i != PCKT_SIZE; i++) {
      n = read(file_fd, packet + i, 1);
      //printf("%s\n", packet);
      if (n == 0) { //reached EOF
	data = true;
	break;
      }
      if (n < 0)
	error("ERROR writing to packet buffer");
      count++;
    }

    n = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, clilen);
   }
  
  //close connection
  close(sockfd);

  return 0;
}

