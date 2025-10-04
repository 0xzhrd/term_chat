# _*_ makefile _*_

CC = gcc
all: chat

chat: main.o server.o client.o  
	gcc main.o server.o client.o -o chat

main.o: main.c headers.h
	gcc -Wall -c main.c

server.o: server.c headers.h
	gcc -Wall -c server.c

client.o: client.c headers.h
	gcc -Wall -c client.c


