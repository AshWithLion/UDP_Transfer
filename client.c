//Code for Client side
/*
   Udp client 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>


void error(char *message) {
    perror(message);
    exit(0);
}

int main(int argc, char* argv[]) {
    int socketfd, port_number, current;
    struct sockaddr_in server_address;
    struct hostent *server;
    char *host_name, *file_name;
    double loss, corruption;
    socklen_t server_length;
    FILE *file;

    // check argument orders
    if (argc < 6) {
        error("You should type ./client hostname port filename PLoss PCorruption\n");
        exit(1);
    }

    host_name = argv[1];
    port_number = atoi(argv[2]);
    file_name = argv[3];
    loss = atof(argv[4]);
    corruption = atof(argv[5]);

    if (port_number < 0)
    {
        error("Port Number shoould be > 0\n");
    }
    if (loss < 0.0 || loss > 1.0 || corruption < 0.0 || corruption > 1.0)
    {
        error("Probabilities for loss or corruption should be located between 0.0 and 1.0\n");
    }

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

   
    // send request
    if (sendto(socketfd, file_name, sizeof(file_name), 0, (struct sockaddr*) &server_address, server_length) < 0)\
    {
        error("ERROR sending request\n");
    }

    printf("Exiting client\n");
    close(socketfd);
    return 0;
}