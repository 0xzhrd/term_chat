# _*_ makefile _*_

CC = gcc
all: chat

chat: main.o server.o client.o transfers.o terminal.o logic.o
	gcc main.o server.o client.o transfers.o terminal.o logic.o -o chat 

main.o: main.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c main.c 

server.o: server.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c server.c 

client.o: client.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c client.c 

transfers.o: transfers.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c transfers.c 

terminal.o: terminal.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c terminal.c

logic.o: logic.c headers.h
	gcc -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L -c logic.c

