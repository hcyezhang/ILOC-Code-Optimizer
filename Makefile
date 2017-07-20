CC=g++
CFLAGS=-O2 -std=c++11

all: optimizer-unroll optimizer-svn

optimizer-unroll:
	$(CC) $(CFLAGS) -o optimizer-unroll optimizer-unroll.cpp

optimizer-svn:
	$(CC) $(CFLAGS) -o optimizer-svn optimizer-svn.cpp

clean:
	rm optimizer-unroll optimizer-svn
