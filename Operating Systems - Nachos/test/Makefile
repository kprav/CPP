# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles testsuite3 testsuite1 testsuite2 testsuite2_1 testsuite2_2 testsuite2_3 senate helloworld client1 client2 client3 client4 client5 President Operator Senator Visitor Visitor2

start.o: start.s ../userprog/syscall.h senate.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles


testsuite1.o: testsuite1.c
	$(CC) $(CFLAGS) -c testsuite1.c
testsuite1: testsuite1.o start.o
	$(LD) $(LDFLAGS) start.o testsuite1.o -o testsuite1.coff
	../bin/coff2noff testsuite1.coff testsuite1

testsuite2.o: testsuite2.c
	$(CC) $(CFLAGS) -c testsuite2.c
testsuite2: testsuite2.o start.o
	$(LD) $(LDFLAGS) start.o testsuite2.o -o testsuite2.coff
	../bin/coff2noff testsuite2.coff testsuite2

testsuite2_2.o: testsuite2_2.c
	$(CC) $(CFLAGS) -c testsuite2_2.c
testsuite2_2: testsuite2_2.o start.o
	$(LD) $(LDFLAGS) start.o testsuite2_2.o -o testsuite2_2.coff
	../bin/coff2noff testsuite2_2.coff testsuite2_2

testsuite2_1.o: testsuite2_1.c
	$(CC) $(CFLAGS) -c testsuite2_1.c
testsuite2_1: testsuite2_1.o start.o
	$(LD) $(LDFLAGS) start.o testsuite2_1.o -o testsuite2_1.coff
	../bin/coff2noff testsuite2_1.coff testsuite2_1

testsuite2_3.o: testsuite2_3.c
	$(CC) $(CFLAGS) -c testsuite2_3.c
testsuite2_3: testsuite2_3.o start.o
	$(LD) $(LDFLAGS) start.o testsuite2_3.o -o testsuite2_3.coff
	../bin/coff2noff testsuite2_3.coff testsuite2_3

testsuite3.o: testsuite3.c
	$(CC) $(CFLAGS) -c testsuite3.c
testsuite3: testsuite3.o start.o
	$(LD) $(LDFLAGS) start.o testsuite3.o -o testsuite3.coff
	../bin/coff2noff testsuite3.coff testsuite3

senate.o: senate.c
	$(CC) $(CFLAGS) -c senate.c
senate: senate.o start.o
	$(LD) $(LDFLAGS) start.o senate.o -o senate.coff
	../bin/coff2noff senate.coff senate

helloworld.o: helloworld.c
	$(CC) $(CFLAGS) -c helloworld.c
helloworld: helloworld.o start.o
	$(LD) $(LDFLAGS) start.o helloworld.o -o helloworld.coff
	../bin/coff2noff helloworld.coff helloworld

client1.o: client1.c
	$(CC) $(CFLAGS) -c client1.c
client1: client1.o start.o
	$(LD) $(LDFLAGS) start.o client1.o -o client1.coff
	../bin/coff2noff client1.coff client1

client2.o: client2.c
	$(CC) $(CFLAGS) -c client2.c
client2: client2.o start.o
	$(LD) $(LDFLAGS) start.o client2.o -o client2.coff
	../bin/coff2noff client2.coff client2
client3.o: client3.c
	$(CC) $(CFLAGS) -c client3.c
client3: client3.o start.o
	$(LD) $(LDFLAGS) start.o client3.o -o client3.coff
	../bin/coff2noff client3.coff client3

client4.o: client4.c
	$(CC) $(CFLAGS) -c client4.c
client4: client4.o start.o
	$(LD) $(LDFLAGS) start.o client4.o -o client4.coff
	../bin/coff2noff client4.coff client4

client5.o: client5.c
	$(CC) $(CFLAGS) -c client5.c
client5: client5.o start.o
	$(LD) $(LDFLAGS) start.o client5.o -o client5.coff
	../bin/coff2noff client5.coff client5

President.o: President.c
	$(CC) $(CFLAGS) -c President.c
President: President.o start.o
	$(LD) $(LDFLAGS) start.o President.o -o President.coff
	../bin/coff2noff President.coff President

Operator.o: Operator.c
	$(CC) $(CFLAGS) -c Operator.c
Operator: Operator.o start.o
	$(LD) $(LDFLAGS) start.o Operator.o -o Operator.coff
	../bin/coff2noff Operator.coff Operator

Senator.o: Senator.c
	$(CC) $(CFLAGS) -c Senator.c
Senator: Senator.o start.o
	$(LD) $(LDFLAGS) start.o Senator.o -o Senator.coff
	../bin/coff2noff Senator.coff Senator

Visitor.o: Visitor.c
	$(CC) $(CFLAGS) -c Visitor.c
Visitor: Visitor.o start.o
	$(LD) $(LDFLAGS) start.o Visitor.o -o Visitor.coff
	../bin/coff2noff Visitor.coff Visitor

Visitor2.o: Visitor2.c
	$(CC) $(CFLAGS) -c Visitor2.c
Visitor2: Visitor2.o start.o
	$(LD) $(LDFLAGS) start.o Visitor2.o -o Visitor2.coff
	../bin/coff2noff Visitor2.coff Visitor2

clean:
	rm -f *.o *.coff
