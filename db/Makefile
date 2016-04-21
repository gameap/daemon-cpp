# CC=g++
CC=i686-w64-mingw32-gcc-win32
CFLAGS=-lmine -c -Wall -std=c++11

all: mysql.o
	$(CC) mysql.o -shared -o mysql.so -lmysqlclient
	make clean

mysql.o:
	$(CC) $(CFLAGS) mysql.cpp -lm -fPIC

clean:
	rm -rf *.o
