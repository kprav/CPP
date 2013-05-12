all: server client common.o
CC = g++
CFLAGS = -c
OBJS = server.o common.o
OBJC = client.o common.o
LLIBS = -L/home/scf-22/csci551b/openssl/lib -lcrypto -lsocket -lresolv -lnsl -Wno-deprecated
CLIBS = -I/home/scf-22/csci551b/openssl/include -Wno-deprecated

server: $(OBJS)
	$(CC) -o server $(OBJS) $(LLIBS)

client: $(OBJC)
	$(CC) -o client $(OBJC) $(LLIBS)

server.o: server.cpp common.h
	$(CC) $(CFLAGS) server.cpp $(CLIBS) 

client.o: client.cpp common.h
	$(CC) $(CFLAGS) client.cpp $(CLIBS) 

common.o: common.cpp common.h
	$(CC) $(CFLAGS) common.cpp

clean: 
	-rm *.o *~ client server
