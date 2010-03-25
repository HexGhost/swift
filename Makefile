CPPFLAGS=-g -I.

all: swift

swift: swift.o sha1.o compat.o sendrecv.o send_control.o hashtree.o bin64.o bins.o channel.o datagram.o transfer.o httpgw.o cache.o
	g++ -I. *.o -o swift

clean:
	rm *.o swift
