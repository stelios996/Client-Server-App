#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "functions.h"

int main(int argc, char *argv[]){

    char MirrorServerAddr[500], requests[SIZE];
    int MirrorServerPort;

    if(argc != 7){
		perror("Wrong number of arguments: Execution call =\n\t./MirrorInitiator -n <MirrorServerAddress> -p <MirrorServerPort> -s <ContentServerAddress1:ContentServerPort1:dirorfile1:delay1, ...ContentServerAddress2:ContentServerPort2:dirorfile2:delay2, ...>\n");
		return -1;
	}else{
		int i;
		for(i=1; i<argc; i++){					//diabazw ta arguments
			if( strcmp(argv[i], "-n")==0 ){			//briskw -n
				strcpy(MirrorServerAddr, argv[i+1]);
			}else if( strcmp( argv[i], "-p" )==0 ){	//briskw -p
				MirrorServerPort = atoi(argv[i+1]);
			}else if( strcmp( argv[i], "-s" )==0 ){	//briskw -s
				strcpy(requests, argv[i+1]);
			}
		}
	}
    int sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct hostent *h;

    if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) <0 ){     //dhmiourgw ena socket
        perror("Error creating TCP socket");
        return -2;
    }
    if( (h = gethostbyname(MirrorServerAddr) ) == NULL ) {  //briskw th dieuthunsh tou server
        herror("Error gethostbyname ");
        return -3;
    }
    server.sin_family = AF_INET ;
    memcpy(&server.sin_addr, h->h_addr, h->h_length);
    server.sin_port = htons(MirrorServerPort);
    if( connect(sock, serverptr, sizeof(server)) <0 ){      //sundeomai sto server
        perror("Error connecting to server");
        return -4;
    }
    printf("Connecting to %s port %d...\n", MirrorServerAddr, MirrorServerPort);
    printf("Sending request...\n");
    strcat(requests, "\n");
    int i, w, size=strlen(requests)+1;
    for(i=0; i<size; i=i+w){                //stelnw olo to mhnuma ston MirrorServer
        if((w = write(sock, requests+i, size-i)) < 0){
            perror("Error writing request to MirrorServer");
            exit(1);
        }
    }
    printf("Sending request: OK\n");
    printf("Waiting for answers...\n");
//    sleep(5);
    printf("\n\n");
    char ch;
    while( read(sock, &ch, sizeof(char)) > 0){
        if(ch=='\t')
            break;
        printf("%c", ch);
    }
    printf("\n\n");
    close(sock);
    return 0;
}
