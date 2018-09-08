//Ryan Moore, CS 494, Programming Project 1
//Server Side

#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "RM_RCP_H.h"

// Declaring functions
unsigned int file_size(char *);

int main(int argc, char *argv[])
{
	int portn, socks, /*sockc,*/ count;
	struct sockaddr_in serv_address, client_address;
	socklen_t client_length;
	char Sbuffer[MinBuffer]; 
	unsigned int packet_num = 1;
	int buffsize = 0;	
	int file_left = 0;

	// Check the passed in args
	if(argc < 2){
		printf("Error with provided port number.\n");
		exit(0);
	}

	// Transfer the port number to a local variable
	portn = atoi(argv[1]);

	// Create the socket
	if((socks = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("Error in server socket creation.\n");
		exit(0);
	}

	// Identify the socket
	memset((char *)&serv_address, 0, sizeof(serv_address));
	serv_address.sin_family = AF_INET;
	serv_address.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_address.sin_port = htons(portn);

	// Binding the server socket
	if(bind(socks, (struct sockaddr *)&serv_address, sizeof(serv_address)) < 0){
		printf("Error binding socket.\n");
		exit(0);
	}	

	// Now listen for a conection.
	listen(socks, 5);

	// Accept the connection
	client_length = sizeof(client_address);
/*	sockc = accept(socks, (struct sockaddr *)&client_address, &client_length);
	delay(5);
	if(sockc < 0){
		printf("Error when accepting the connection.\n");
		//exit(0);
	}
	else{
		printf("server side socket connection made.\n");
	}
*/	

	// Receive the first packet
	count = recvfrom(socks, Sbuffer, MinBuffer, 0,(struct sockaddr *)&client_address, &client_length);
	if(count < 0){
		printf("Error receiving the intial packet.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// Deserialize the packet
	Frame rec_hi = malloc(sizeof(Frame));
	rec_hi = deserialize(Sbuffer);
	//Test to make sure we have something
	printf("First received packet by server has seq number: %d\n.", rec_hi->seq_num);

	// Reply to the connection with an ACK packet
	// Which is just the seq number plus one
	Frame server_ack = malloc(sizeof(Frame));
	server_ack->frame_type = 2;
	server_ack->seq_num = packet_num;
	server_ack->ack = rec_hi->seq_num + 1;  
	packet_num += 1;

	// Serialize the packet
	char * first_ack = serialize(server_ack);
	char f_ack[sizeof(first_ack)];
	strcpy(f_ack, first_ack);

	// Send it
	count = sendto(socks, f_ack, sizeof(f_ack), 0, (struct sockaddr *)&client_address, client_length);
	if(count < 0){
		printf("Error sending the first ACK packet.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// Clear the buffer
	memset(Sbuffer, '\0', sizeof(Sbuffer));

	// Recieve the file path from the client and check it
	count = recvfrom(socks, Sbuffer, MinBuffer, 0,(struct sockaddr *)&client_address, &client_length);
//	delay(5);
	if(count < 0){
		printf("Error receiving the file request packet.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// Deserialize packet
	Frame req_path = malloc(sizeof(Sbuffer));	
	req_path = deserialize(Sbuffer);

	//Check the file name
	printf("The file name is: %s\n.", req_path->file_path);

	// Send the file size to the client
	Frame FSize = malloc(sizeof(Frame));
	FSize->frame_type = 4; 	
	FSize->seq_num = packet_num;
	packet_num += 1;
	FSize->file_size = file_size(req_path->file_path);

	if(FSize->file_size == -1){
		printf("Closing connection, couldn't locate the file.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// open the file
	FILE *fp = NULL;
	if(NULL == (fp = fopen(req_path->file_path,"r")))
	{
		printf("Error opening the file.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// Serialize the packet
	char * FS = serialize(FSize);
	char f_s[sizeof(FS)];
	strcpy(f_s, FS);


	count = sendto(socks, f_s, sizeof(f_s), 0, (struct sockaddr *)&client_address, client_length);
	if(count < 0){
		printf("Error sending the file size packet.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	// Get the acknowledgement for the file size
	// Clear the buffer
	memset(Sbuffer, '\0', sizeof(Sbuffer));

	// Recieve the file size ACK from the client and check it
	count = recvfrom(socks, Sbuffer, MinBuffer, 0,(struct sockaddr *)&client_address, &client_length);
	if(count < 0){
		printf("Error receiving the ACK for the file size packet.\n");
		shutdown(socks, SHUT_RDWR);
		exit(0);
	}

	//Frame size_ack = malloc(sizeof(Sbuffer));	
	//size_ack = deserialize(Sbuffer);

	printf("ACK received for the file size. We will now begin to send the file.\n");

	//int ackn = size_ack->ack;

	// Create some packetss for the loop
	// For sending the file
	Frame Send_File = malloc(sizeof(Frame));
	// For receiveing Acks
	Frame AckData = malloc(sizeof(Frame));
	
	// Start the loop that send the file to the client	
	while(file_left <= FSize->file_size)
	{
		// Clear buffer and packets from last loop
		memset(Sbuffer, '\0', sizeof(Sbuffer));
		clear_frame(Send_File);
		clear_frame(AckData);

		// Fill a packet with data
		Send_File->frame_type = 5;
		Send_File->seq_num = packet_num;
		packet_num += 1;
		buffsize += fread((void*)&Send_File->data, 1, sizeof(&Send_File->data) - 1, fp);			

		// Serialize the packet
		char *SFile = serialize(Send_File);
		char S_File[sizeof(SFile)];
		strcpy(S_File, SFile);

		// Send the data, loop if we dont get an ack in time
		do{
			sendto(socks, SFile, sizeof(SFile), 0, (struct sockaddr *)&client_address, client_length);

			// Wait for an ack, if not gotten in time, sen packet again	
			count = recvfrom(socks, Sbuffer, MinBuffer, 0,(struct sockaddr *)&client_address, &client_length);
			if(delay(TIMEOUT) == -1 || count == -1)
			{
				count = -1;
				printf("ACK not received in time, retransmitting.\n"); 
			}
			else
			{
				AckData = deserialize(Sbuffer);
				//ackn = AckData->ack;
			}
		}while(count == -1);
	}
	
	shutdown(socks, SHUT_RDWR);
	return 0;
}



// Function to check if a file exsists and get its size
unsigned int file_size(char * name){
	// Variables
	struct stat file;
	unsigned int size;

	// Get its size, if we get a -1, file doesnt exsist
	if(stat(name, &file) == -1){
		printf("Error finding file.\n");
		return -1;
	}
	
	// Convert the size to an unsigned int
	size = file.st_size;
	printf("File size(uint form): %d\n", size);

	// Return the size
	printf("File size in bytes: %jd.\n", file.st_size);

	return size;
}
