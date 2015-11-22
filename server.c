
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <signal.h>/* signal name macros, and the kill() prototype */

#define BUFSIZE 2048;

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  char buffer[BUFSIZE];

  memset(buffer, 0, BUFSIZE);//reset memory

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

  //read client's message


  //close connection
  close(sockfd);

  return 0;
}

