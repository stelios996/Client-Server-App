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
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include "functions.h"

static int ContentServerID=0;

void *MirrorManager(void *arg){

    char *request = (char *) arg;
//    printf("MM thread %lu: %s\n", pthread_self(), request);
    char buff[500];
    strcpy(buff, request);
//    printf("%s\n", buff);
    char *cserveraddr = strtok(buff, " :\n");
    int csport = atoi(strtok(NULL, " :\n"));
    char *dirorfile = strtok(NULL, " :\n");
    int delay = atoi(strtok(NULL, " :\n"));
    printf("\nMirrorManager thread %lu requests:\t%s\t%d\t%s\t%d\n", pthread_self(), cserveraddr, csport, dirorfile, delay);

    int sock;
    struct sockaddr_in cserver;
    struct sockaddr *cserverptr = (struct sockaddr *) &cserver;
    struct hostent *h;

    if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) <0 ){     //dhmiourgw ena socket
        perror("Error creating TCP socket");
        return NULL;
    }
    if( (h = gethostbyname(cserveraddr) ) == NULL ) {  //briskw th dieuthunsh tou content server
        herror("Error gethostbyname ");
        return NULL;
    }
    cserver.sin_family = AF_INET ;
    memcpy(&cserver.sin_addr, h->h_addr, h->h_length);
    cserver.sin_port = htons(csport);
    if( connect(sock, cserverptr, sizeof(cserver)) <0 ){      //sundeomai sto content server
        perror("Error connecting to content server");
        return NULL;
    }
    printf("MirrorManager thread %lu connecting to %s port %d...\n", pthread_self(), cserveraddr, csport);
    printf("MirrorManager thread %lu sending request...\n", pthread_self());

    char list_request[100];
    int req_id = ContentServerID;
    sprintf(list_request, "LIST %d %d\n", ContentServerID++, delay);    //dhmiourgw to aithma LIST pros apostolh
	printf("%s\n",list_request);

    int i, w, size=strlen(list_request)+1;
    for(i=0; i<size; i=i+w){                //stelnw olo to aithma LIST ston ContentServer
        if((w = write(sock, list_request+i, size-i)) < 0){
            perror("Error writing request to content server");
            exit(1);
        }
    }
    printf("MirrorManager thread %lu sending request: OK\n", pthread_self());
    printf("MirrorManager thread %lu waiting for answers...\n", pthread_self());

    char b[1],cslist[SIZE];
    i=0;
    while( read(sock, b, 1) > 0 )                   //diabazw to mhnuma pou stelnei o content server
        cslist[i++]=b[0];
//    printf("Receiving list of directories/files from %s : OK\n%s", cserveraddr, cslist);
    printf("MirrorManager thread %lu receiving list of directories/files from %s : OK\n", pthread_self(), cserveraddr);
    close(sock);

    int fcount=0;
    for(i=0; i<(strlen(cslist)+1); i++){    //metraw to plhthos twn stoixeiwn ths listas dir/files pou esteile o content server
        if(cslist[i]=='\n')
            fcount++;
    }
    int j=0, filestowrite=0;
    char files[fcount][MAXSIZE];
    char *token = strtok(cslist, " \n");
    while(token!=NULL){                         //apothikeuw se pinaka to path kathe stoixeiou ths listas
        if(strstr(token, dirorfile) != NULL){   //pou periexei to dirorfile pou zhta o MirrorInitiator
            strcpy(files[j++], token);
            filestowrite++;
        }
        token = strtok(NULL, " \n");
    }
    pthread_mutex_lock(&reqmtx);
    numrequests = numrequests+filestowrite;
    pthread_mutex_unlock(&reqmtx);

    printf("\nMirrorManager thread %lu files:\n", pthread_self());
    for(j=0; j<filestowrite; j++)       //auta einai ta arxeia pou zhthse o MirrorInitiator sth sugkekrimenh aithsh
        printf("\t%s\n", files[j]);

    char rbuff[MAXSIZE];
    for(j=0; j<filestowrite; j++){
                                //grafw to kathe path stoixeiou mazi me kapoies plhrofories sto buffer
        writer_lock();
        pool.b_end = (pool.b_end+1)%MAXFILES;
        sprintf(rbuff, "%s,%s,%d,%d,%s", files[j], cserveraddr, csport, req_id, dirorfile);
        strcpy(pool.buffer[pool.b_end], rbuff);
        pool.b_count++;
        printf("Mirror Manager thread %lu wrote to buffer: %s\n", pthread_self(), rbuff);
        writer_unlock();
    }

    pthread_mutex_lock(&crbmtx);
    numContentsWroteB++;
    pthread_mutex_unlock(&crbmtx);

    pthread_exit(NULL);
}

void *Worker(void *arg){

    char *dirname = (char *) arg;
    printf("Creating Worker thread %lu dirname: %s\n", pthread_self(), dirname);

    char file[MAXSIZE];

    int sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct hostent *h;

    char *cserveraddr, *dirorfile, fetch_request[500], *init_req;
    int csport, csreq_id;
    for( ; ;){
                //diabazw ena stoixeio apo to buffer
        reader_lock();
        strcpy(file, pool.buffer[pool.b_start]);
        pool.b_start = (pool.b_start+1)%MAXFILES;
        pool.b_count--;
        printf("Worker thread %lu read from buffer: %s\n", pthread_self(), file);
        reader_unlock();

        dirorfile = strtok(file, " ,\n");
        cserveraddr = strtok(NULL, " ,\n");
        csport = atoi(strtok(NULL, " ,\n"));
        csreq_id = atoi(strtok(NULL, " ,\n"));
        init_req = strtok(NULL, " ,\n");


        if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) <0 ){     //dhmiourgw ena socket
        	perror("Error creating TCP socket");
       		exit(1);
    	}
        if( (h = gethostbyname(cserveraddr) ) == NULL ) {  //briskw th dieuthunsh tou server
            herror("Error gethostbyname ");
            exit(2);
        }
        server.sin_family = AF_INET ;
        memcpy(&server.sin_addr, h->h_addr, h->h_length);
        server.sin_port = htons(csport);
        if( connect(sock, serverptr, sizeof(server)) <0 ){      //sundeomai sto server
            perror("Error connecting to server");
            exit(3);
        }
        printf("Worker thread %lu connecting to %s port %d...\n", pthread_self(), cserveraddr, csport);
        printf("Worker thread %lu sending request...\n", pthread_self());
        sprintf(fetch_request, "FETCH %s %d\n", dirorfile, csreq_id);   //dhmiourgw to aithma FETCH pros apostolh
        int i, w, size=strlen(fetch_request)+1;
        for(i=0; i<size; i=i+w){                //stelnw to aithma FETCH ston Content Server
            if((w = write(sock, fetch_request+i, size-i)) < 0){
                perror("Error writing request to ContentServer");
                exit(4);
            }
        }
        printf("Worker thread %lu sending request: OK\n", pthread_self());
        printf("Worker thread %lu waiting for answers...\n", pthread_self());

        char content_name[MAXSIZE], dir[MAXSIZE];
        sprintf(content_name, "%s_%d", cserveraddr, csport);
        sprintf(dir,"%s/%s", dirname, content_name);
        if(mkdir(dir, 0777) == -1){     //dhmiourgw to fakelo dirname/content_name
            if(errno == EEXIST){
                printf("Directory already exists\n");
            }else{
                perror("Error creating directory");
                exit(5);
            }
        }
        int pathmode;
        if(read(sock, &pathmode, sizeof(int)) <0 ){     //diabazw ton tupo tou stoixeiou pou perigrafei to path (fakelos/arxeio)
            perror("Error reading pathmode from content server");
            exit(6);
        }
        if(pathmode == 0){  //directory
            	char *ret = strstr(dirorfile, init_req);
            	char directory[MAXSIZE];
            	sprintf(directory, "%s/%s", dir, ret);
            	if(mkdir(directory, 0777) == -1){   //to stoixeio einai fakelos, ton dhmiourgw epitopou
                	if(errno == EEXIST){
                    		printf("Directory already exists\n");
                	}else{
                    		perror("Error creating directory");
                    		exit(7);
                	}
            	}
        }else if(pathmode == 1){    //file
            char *ret = strstr(dirorfile, init_req);
            char filepath[MAXSIZE];
            sprintf(filepath, "%s/%s", dir, ret);
            int fd;                 //to stoixeio einai arxeio, to dhmiourgw gia write
            if((fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1){
                perror("Error creating file");
                exit(8);
            }
            int w2;
            char b;
            while(read(sock, &b, sizeof(char))>0){      //diabazw apo to socket to arxeio xarakthra-xarakthra
                if(b=='\0'){
                    break;      //to arxeio eftase sto telos, kanw break
                }

                pthread_mutex_lock(&bytesmtx);
                bytesTransferred++;
                pthread_mutex_unlock(&bytesmtx);

                if((w2 = write(fd, &b, sizeof(char))) <0){  //grafw to xarakthra pou diabasa apo to socket sto arxeio
                    perror("Error receiving file");
                    exit(9);
                }
            }
            printf("\nReceiving file OK\n");
            close(fd);

            pthread_mutex_lock(&filesmtx);
            filesTransferred++;
            pthread_mutex_unlock(&filesmtx);

//	    printf("FilesTransferred: %d ,BytesTransferred: %d\n",filesTransferred, bytesTransferred);
        }

        pthread_mutex_lock(&servedmtx);
        numserved++;
        pthread_mutex_unlock(&servedmtx);

        close(sock);

        if((numserved == numrequests) && (numContentsWroteB == numContentDevices)){
		AllDone=1;
        }

//        printf("FilesTransferred: %d ,BytesTransferred: %d\n",filesTransferred, bytesTransferred);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]){

    int port, threadnum;
    char dirname[200];

    if(argc != 7){
		perror("\nWrong number of arguments: Execution call =\n\t./MirrorServer -p <port> -m <dirname> -w <threadnum>\n");
		exit(1);
	}else{
		int i;
		for(i=1; i<argc; i++){					//diabazw ta arguments
			if( strcmp(argv[i], "-p")==0 ){			//briskw -p
				port = atoi(argv[i+1]);
			}else if( strcmp( argv[i], "-m" )==0 ){	//briskw -m
				strcpy(dirname, argv[i+1]);
			}else if( strcmp( argv[i], "-w" )==0 ){	//briskw -w
				threadnum = atoi(argv[i+1]);
			}
		}
	}

    AllDone = 0;

    init_pool(&pool);
    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&writer_cond, 0);
    pthread_cond_init(&reader_cond, 0);
	writers=0;
	readers=0;

    pthread_mutex_init(&bytesmtx, 0);
    pthread_mutex_init(&filesmtx, 0);
    bytesTransferred = 0;
    filesTransferred = 0;

    pthread_mutex_init(&crbmtx, 0);
    pthread_mutex_init(&reqmtx, 0);
    pthread_mutex_init(&servedmtx, 0);
    numContentsWroteB = 0;
    numrequests = 0;
    numserved = 0;

    int sock, csock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    struct hostent *c;

    if( (sock = socket(AF_INET, SOCK_STREAM, 0) ) <0 ){     //dhmiourgw ena socket
        perror("Error creating TCP socket");
        exit(2);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if( bind(sock, serverptr, sizeof(server)) <0 ){        //kanw bind to socket sto adress
        perror("Error binding TCP socket");
        exit(3);
    }
    if( listen(sock, 5) <0 ){                               //kanw listen gia sundeseis
        perror("Error listening TCP socket");
        exit(4);
    }
    printf("Waiting for requests...\n");

    int i, j, rcount=0;
    char buff[1], req[2000];
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

    while( read(csock, buff, 1) > 0 ){                   //diabazw to mhnuma pou stelnei o client
	if(buff[0]=='\n')
		break;
        req[i++]=buff[0];
    }
//    printf("MESSAGE:\n\t%s\n", req);

        for(i=0; i<(strlen(req)+1); i++){					//upologizw poses aithseis se content servers exw
            if(req[i] == ',')
                rcount++;
        }

        numContentDevices = rcount+1;

//        printf("NUMBER OF MIRROR MANAGER THREADS = %d\n",rcount+1);
        char csrequests[rcount+1][500];
        char *token;
        token = strtok(req, " ,\n");			//spaw to mhnuma pou esteile o MirrorInitiator se aithseis pros content servers
        j=0;
        while(token!=NULL){
            strcpy(csrequests[j],token);
            token = strtok(NULL, " ,\n");
            j++;
        }

        int err1, mmthreads = rcount+1;
        pthread_t *tids;
        if((tids = malloc(mmthreads*sizeof(pthread_t))) == NULL){
            perror("Error allocating Mirror Manager threads");
            exit(7);
        }
        for(i=0; i<mmthreads; i++){     //dhmiourgw ta MirrorManager threads
            if(err1 = pthread_create(tids+i, NULL, MirrorManager, (void *) csrequests[i])){
                perror("Error creating Mirror Manager threads");
                exit(8);
            }
        }

        int err2, k, workers = threadnum;
        pthread_t *workerids;
        if((workerids = malloc(workers*sizeof(pthread_t))) == NULL){
            perror("Error allocating Worker threads");
            exit(9);
        }
        for(k=0; k<workers; k++){               //dhmiourgw ta Worker threads
            if(err2 = pthread_create(workerids+k, NULL, Worker, (void *) dirname)){
                perror("Error creating Worker threads");
                exit(10);
            }
        }

        for(i=0; i<mmthreads; i++){
            if(err1 = pthread_join(*(tids+i), NULL)){
                perror("Error joining Mirror Manager threads");
                exit(11);
            }
        }
//	printf("MIRROR MANAGER THREADS EXIT OK !!!!!!\n");
//        for(k=0; k<workers; k++){
//            if(err2 = pthread_join(*(workerids+k), NULL)){
//                perror("Error joining Worker threads");
//                exit(12);
//            }
//        }

	while(AllDone!=1){}	//perimenw na ikanopoih8oun oles oi aithseis

        char results[MAXSIZE];
        sprintf(results, "Devices done : %d\nBytes transferred : %d\nFiles transferred : %d\nAverage file transfer size : %f\n\t", numContentsWroteB, bytesTransferred, filesTransferred, (float) bytesTransferred/filesTransferred);
//	printf("%s", results);
        int res_size = strlen(results)+1, y;
        for(j=0; j<res_size; j=j+y){
            if((y = write(csock, results+j, res_size-j)) < 0){
                perror("Error writing to MirrorInitiator");
                exit(13);
            }
        }

    pthread_cond_destroy(&writer_cond);
    pthread_cond_destroy(&reader_cond);
    pthread_mutex_destroy(&mtx);

    pthread_mutex_destroy(&bytesmtx);
    pthread_mutex_destroy(&filesmtx);

    pthread_mutex_destroy(&crbmtx);
    pthread_mutex_destroy(&reqmtx);
    pthread_mutex_destroy(&servedmtx);

    return 0;
}
