//Code for Client side

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

double simulation() {
    return (double) rand()/(double) RAND_MAX;
}

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
    char buffer_out[PCKT_SIZE];
    char buffer_in[PCKT_SIZE];
    FILE *file_for_pkt;
    
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
    
    // build request
    header_t* out_pck = (header_t*) &buffer_out;
    char* payload_out = buffer_out + 4;

    printf("Building request for %s\n", file_name);

    out_pck->type = REQ;
    out_pck->seq_num = 0;
    strcpy(payload_out, file_name);
    out_pck->size = sizeof(payload_out);
    

    //header_t out_pck, in_pck;
    /*
    printf("Building request for %s\n", file_name);
    memset((char *) &out_pck, 0, sizeof(out_pck));
    out_pck.type = REQ;
    out_pck.seq_num = 0;
    out_pck.size = strlen(file_name) + 1;
    strcpy(out_pck.data, file_name);
    */

    server_length = sizeof(server_address);
    //set space for server_address
    memset((char *) &server_address, 0, server_length);
    //IPv4
    server_address.sin_family = AF_INET;
    
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
    server_address.sin_port = htons(port_number);
    
    // send the request
    if (sendto(socketfd, buffer_out, sizeof(buffer_out), 0, (struct sockaddr*) &server_address, server_length) < 0)
    {
        error("ERROR sending request\n");
        
    }
    printf("File requested: %s\n", payload_out);
    
    current_position = 1;
    file_for_pkt = fopen(strcat(file_name, "_copy"), "wb");
    
    // ACK preparing
    /*memset((char *) &out_pck, 0, sizeof(out_pck));
    out_pck.seq_num = current_position - 1;
    out_pck.type = ACK; // ACK packet
    out_pck.size = 0;*/
    
    //memset(&buffer_out, 0, sizeof(buffer_out));
    out_pck = (header_t*) &buffer_out;
    //memset((char *) &out_pck, 0, sizeof(out_pck));
    out_pck->type = ACK;
    out_pck->seq_num = current_position-1;
    out_pck->size = 0;

    /*
    char* ACK_packet[4];
    header_t* ACK_head = (header_t*) &ACK_packet;
    int ACK_number = 0;
    ACK_head->type = ACK;
    ACK_head->seq_num = 0;
    ACK_head->size = 0;
   */
    srand(time(0));
    
    header_t* in_pck = (header_t*) &buffer_in;
    char* payload_in = buffer_in + 4;
    

    // Watching responses
    while (1) {
        if (recvfrom(socketfd, buffer_in, sizeof(buffer_in),0, (struct sockaddr*) &server_address, &server_length) < 0) {
            printf("We lost packet\n");
        } else {
        //printf("Buffer in: %d\n", in_pck->seq_num);
            // Use simulated situation: loss & corruption
            if (simulation() < loss) {
                printf("Packet Loss occurred: %d\n", in_pck->seq_num);
                continue;
            } else if (simulation() < corruption) {
                printf("Packet Corruption occurred: %d\n", in_pck->seq_num);
                // send again the last ACK
                if (sendto(socketfd, buffer_out, sizeof(buffer_out), 0, (struct sockaddr*) &server_address, server_length) < 0) {
                    error("There is an error while resending ACK\n");
                }
                printf("ACKed packet: %d\n", out_pck->seq_num);
                continue;
            }
           // printf("ACKed packet: %d\n", out_pck->seq_num);
            
            // See number of packet and type
           /* if (in_pck->seq_num > current_position) {
                printf("Ignore this sequence number %d: should be %d\n", in_pck->seq_num, current_position);
                continue;
            } else if (in_pck->seq_num < current_position) {
                printf("Ignore this sequence number %d: should be %d\n", in_pck->seq_num, current_position);
                out_pck->seq_num = in_pck->seq_num;
            } else {*/
                // Fin signal is arrived
                if (in_pck->type == FIN) {
            printf("FIN received. break");
                    break;
                } else if (in_pck->type != DATA) {
                    printf("Ignore this packet %d: not a data packet\n", in_pck->seq_num);
                    continue;
                }
                fwrite(payload_in, 1, in_pck->size, file_for_pkt);
                out_pck->seq_num = in_pck->seq_num;
                current_position++;
        printf("current position: %d\n", current_position);
                if (sendto(socketfd, buffer_out, sizeof(buffer_out), 0, (struct sockaddr*) &server_address, server_length) < 0) {
                    error ("There is an error while sending ACK\n");
                }
                printf("Currently ACKed packet: %d\n", out_pck->seq_num);          
            //}
        }
        
    }
    
    // Send FIN signal
    /*
    memset((char *) &out_pck, 0, sizeof(out_pck));
    out_pck.type = FIN;
    out_pck.size = 0;
    out_pck.seq_num = current_position;
    */
    memset((char *) &out_pck, 0, sizeof(out_pck));
    out_pck->type = FIN;
    out_pck->size = 0;
    out_pck->seq_num = current_position;

    if (sendto(socketfd, buffer_out, sizeof(buffer_out), 0, (struct sockaddr*) &server_address, server_length) < 0) {
        error("There is an error while sending FIN\n");
    }
    printf("ACK'd packet: %d", out_pck->seq_num);
    
    printf("Exiting client\n");
    fclose(file_for_pkt);
    close(socketfd);
    return 0;
}