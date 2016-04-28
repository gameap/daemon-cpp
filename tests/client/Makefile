CC=g++
# CC=i686-w64-mingw32-g++
CFLAGS=-I../../ -lmine -c -Wall -Wno-write-strings -std=c++11

all: objects
	$(CC) gcrypt.o client_sendfiles.o crypto.o base64.o -o client_sendfiles -lbinn -lboost_system -lpthread -lcrypto -lcryptopp
	$(CC) gcrypt.o client_readdir.o crypto.o base64.o -o client_readdir -lbinn -lboost_system -lpthread -lcrypto -lcryptopp
# 	$(CC) gcrypt.o client.o -o client -lbinn -lboost_system -lpthread -lcrypto -lcryptopp

objects:
	$(CC) $(CFLAGS) -I ../../ -c ../../functions/gcrypt.cpp -o gcrypt.o 
	$(CC) $(CFLAGS) -c ../../classes/base64.cpp -o base64.o 
	$(CC) $(CFLAGS) -c ../../classes/crypto.cpp -o crypto.o 
	$(CC) $(CFLAGS) -c client_sendfiles.cpp -o client_sendfiles.o 
	$(CC) $(CFLAGS) -c client_readdir.cpp -o client_readdir.o 
