#
# iniparser Makefile
#
all: node
CC      = g++
CFLAGS  = -Wall -g -c -I/home/scf-22/csci551b/openssl/include
RM      = rm -f
LLIBS = -L/home/scf-22/csci551b/openssl/lib -lcrypto -lpthread -lm -lsocket -lnsl

# Implicit rules

node: node.o iniparser.o processData.o
	$(CC) dictionary.o strlib.o iniparser.o processData.o node.o -o node $(LLIBS)

node.o: node.h node.cpp iniparser.o
	$(CC) $(CFLAGS) node.cpp

iniparser.o: dictionary.o iniparser.h iniparser.c
	g++ $(CFLAGS) iniparser.c 

strlib.o: strlib.h strlib.c
	g++ $(CFLAGS) strlib.c

dictionary.o: dictionary.h strlib.h dictionary.c strlib.o
	g++ $(CFLAGS) dictionary.c

processData.o: processData.cpp 
	$(CC) $(CFLAGS) processData.cpp

clean:
	-rm *.o node


	
