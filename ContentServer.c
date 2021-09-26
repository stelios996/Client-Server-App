#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "functions.h"

int main(int argc, char *argv[]){

    int csport;
    char dirorfilename[300];

    if(argc != 5){
        perror("\nWrong number of arguments: Execution call =\n\t./ContentServer -p <port> -d <dirorfilename>\n");
        exit(1);
    }else{
        int i;
		for(i=1; i<argc; i++){					//diabazw ta arguments
			if( strcmp(argv[i], "-p")==0 ){			//briskw -p
				csport = atoi(argv[i+1]);
			}else if( strcmp( argv[i], "-d" )==0 ){	//briskw -d
				strcpy(dirorfilename, argv[i+1]);
			}
		}
    }

    int sock, csock;
    struct sockaddr_in cserver, client;
    socklen_t clientlen = sizeof(client);
    struct sockaddr *cserverptr = (struct sockaddr *) &cserver;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    struct hostent *c;

    if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) <0 ){     //dhmiourgw ena socket
        perror("Error creating TCP socket");
        exit(2);
    }
    cserver.sin_family = AF_INET;
    cserver.sin_addr.s_addr = htonl(INADDR_ANY);
    cserver.sin_port = htons(csport);
    if( bind(sock, cserverptr, sizeof(cserver)) <0 ){        //kanw bind to socket sto address
        perror("Error binding TCP socket");
        exit(3);
    }
    if( listen(sock, 10) <0 ){                               //kanw listen gia sundeseis
        perror("Error listening TCP socket");
        exit(4);
    }
    printf("Waiting for requests...\n");

    char dlist[SIZE];
    strcpy(dlist, dirorfilename);
    strcat(dlist, "\n");
    list_dir(dirorfilename, dlist); //dhmiourgw th lista me ta arxeia tou dirorfilename pou mporei na steilei o content server
    printf("AVAILABLE LIST DIR/FILES TO SENT:\n%s", dlist);

    int i;
    char buff[1], req[2000];
    struct delays_t *delaylist = NULL;
    for( ; ;){
                                                                //dexomai th sundesh apo ton client
        if( (csock = accept(sock, clientptr, &clientlen )) < 0 ){
            perror("Error accepting connection");
            exit(5);
        }
        if ( (c = gethostbyaddr(( char *)&client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr ), client.sin_family)) == NULL ){	//briskw th dieuthunsh tou client
            herror("Error gethostbyaddr");
            exit(6);
        }
        printf("...Accepted request from %s\n", c->h_name);

        i=0;
        while( read(csock, buff, 1) > 0 ){                   //diabazw to mhnuma pou stelnei o client
            req[i++]=buff[0];
            if(buff[0]=='\n') break;
        }
        req[i]='\0';

        if(strstr(req, "LIST") != NULL){        //an to mhnuma einai aithma LIST
            printf("%s\n", req);
            printf("Sending list of directories/files to %s\n",c->h_name);

            int i, w, size=strlen(dlist)+1;
            for(i=0; i<size; i=i+w){                //stelnw olh th lista sto MirrorServer
                if((w = write(csock, dlist+i, size-i)) < 0){
                    perror("Error writing list to mirror server");
                    exit(7);
                }
            }
            printf("Sending list of directories/files to %s : OK\n",c->h_name);

            strtok(req, " ");
            int req_id = atoi(strtok(NULL, " "));
            int req_delay = atoi(strtok(NULL, " "));
//            printf("%d %d\n", req_id, req_delay);

            struct delays_t *newnode = malloc(sizeof(struct delays_t)); //apothikeuw to id kai to delay sth lista delaylist
            newnode->csid = req_id;                                     //tou sugkekrimenou content server
            newnode->delay = req_delay;                                 //pou periexei to delay gia kathe id
            newnode->next = delaylist;                                  //gia mellontikh xrhsh
            delaylist = newnode;

        }else if(strstr(req, "FETCH") != NULL){     //an to mhnuma einai aithma FETCH
            printf("%s\n", req);
            strtok(req, " ");
            char *dirorfile = strtok(NULL, " ");
            int id = atoi(strtok(NULL, " "));
//            printf("%s %d\n", dirorfile, id);

            int status;
            struct stat st_buff;
            status = stat(dirorfile, &st_buff);
            if( status != 0){
                perror("Error stating path");
                exit(8);
            }
            int pathmode;
            if(S_ISREG(st_buff.st_mode))    //elegxw an to dirorfile einai fakelos h kanoniko arxeio
                pathmode = 1;               //regular file
            else if(S_ISDIR(st_buff.st_mode))
                pathmode = 0;               //directory

            if( write(csock, &pathmode,sizeof(int)) <0 ){       //stelnw sto socket tou worker an prokeitai gia fakelo h arxeio
                perror("Error writing pathmode to MirrorServer");
                exit(9);
            }

            struct delays_t *current=delaylist;
            while(current != NULL){         //anazhtw sth lista delaylist na brw to delay tou sugkekrimenou id
                if(current->csid == id){
                    printf("Sleeping for %d seconds...\n", current->delay);
                    sleep(current->delay);  //o content server koimatai delay seconds prin steilei to arxeio
                    break;
                }
                current = current->next;
            }
            if(pathmode == 1){      //einai arxeio

                FILE *fp;
                if((fp=fopen(dirorfile, "rb"))==NULL){  //anoigw to arxeio
                    perror("Error opening file");
                    exit(10);
                }
                int size1=sizeof(char), w1, j;
                char ch;
                while( (ch=fgetc(fp))!=EOF){     //stelnw xarakthra-xarakthra to arxeio sto socket gia na to parei o worker

                    usleep(10000);
                    if((w1 = write(csock, &ch, size1)) <0 ){
                        perror("Error sending file to content server");
                        exit(9);
                    }
                }

                ch='\0';
                write(csock, &ch, sizeof(ch));
                printf("Sending file OK\n");
                fclose(fp);     //kleinw to arxeio
            }

        }
        close(csock);       //h aithsh ikanopoih8hke, kleinw to socket
    }
}
