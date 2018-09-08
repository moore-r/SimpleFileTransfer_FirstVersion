all: client server

client: RM_RCP_C.c RM_RCP_H.c
	gcc -g -Wall -o client RM_RCP_C.c RM_RCP_H.c

server: RM_RCP_S.c RM_RCP_H.c
	gcc -g -Wall -o server RM_RCP_S.c RM_RCP_H.c
