all: mm2 mm2.o utils.o list.o
CC = g++
CFLAGS = -c
OBJ = mm2.o utils.o list.o
LLIBS = -lpthread -lm -Wall
CLIBS = -g -Wall

mm2: $(OBJ)
	$(CC) -o mm2 $(OBJ) $(LLIBS)

mm2.o: mm2.cpp utils.h list.h
	$(CC) $(CFLAGS) mm2.cpp $(CLIBS)

utils.o: utils.cpp utils.h
	$(CC) $(CFLAGS) utils.cpp $(CLIBS)

list.o: list.cpp list.h
	$(CC) $(CFLAGS) list.cpp $(CLIBS)

clean:
	-rm *.o *~ mm2
