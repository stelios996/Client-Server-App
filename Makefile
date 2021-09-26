CC=gcc

all: MirrorInitiator MirrorServer ContentServer

MirrorInitiator: MirrorInitiator.o
	$(CC) -o MirrorInitiator MirrorInitiator.o

MirrorServer: MirrorServer.o functions.o
	$(CC) -o MirrorServer MirrorServer.o functions.o -lpthread

ContentServer: ContentServer.o functions.o
	$(CC) -o ContentServer ContentServer.o functions.o

MirrorInitiator.o: MirrorInitiator.c
	$(CC) -c MirrorInitiator.c

MirrorServer.o: MirrorServer.c
	$(CC) -c MirrorServer.c

ContentServer.o: ContentServer.c
	$(CC) -c ContentServer.c

functions.o: functions.c
	$(CC) -c functions.c

clean:
	rm -f *.o MirrorInitiator MirrorServer ContentServer
