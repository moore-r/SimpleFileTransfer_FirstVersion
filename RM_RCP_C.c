//Ryan Moore, CS 494, Programming Project 1
//Client Side

#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>

#include "RM_RCP_H.h"

// Declare some functions
unsigned int IP_converter(char *);

int main(int argc, char *argv[])
{
	// Variables
	int Port, sock, ret, count;
	char *IP_Address, *Path; 
	struct sockaddr_in server_address;
	socklen_t server_len;
	char rec_buffer[MinBuffer];
	char data_buffer[SIZE];
	unsigned int packet_number = 1;
	unsigned int file_count = 0;
	int buffptr = 0;

	// Check and make sure we have the correct ammount of args
	if(argc < 4){
		printf("Error in provided directory, IP, or port.\n");
		exit(0);
	}

	// Save the arguments in variables.
	IP_Address = malloc(strlen(argv[1] + 1));
	strcpy(IP_Address, argv[1]);
	Port = atoi(argv[2]);
	Path = malloc(strlen(argv[3] + 1));
	strcpy(Path, argv[3]);

	printf("The IP is: %s, port: %d, Path: %s. \n", IP_Address, Port, Path);

	// Create a socket
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("Error when creating socket.\n");
		exit(0);
	}

	// Identify the socket
	memset((char *)&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = IP_converter(IP_Address);
	server_address.sin_port = htons(Port);

	server_len = sizeof(server_address);

	// Now attempt to connect to the server
	if(connect(sock, (struct sockaddr *)&server_address, server_len) < 0){
		printf("Error connecting to server.\n");
		exit(0);
	}
	else
	{
		printf("Connection made.\n");
	}

	// Send intial message
	Frame hello = malloc(sizeof(Frame));
	hello->frame_type = 1;
	hello->seq_num = packet_number;
	packet_number += 1;

	// Fill buffer with the packet - Serialize the data
	char *connect_buf = serialize(hello);
	char connect[sizeof(connect_buf) + 1];
	strcpy(connect, connect_buf);

	count = sendto(sock, connect, sizeof(connect), 0, (struct sockaddr *)&server_address, server_len);
	if(count == -1){
		printf("Initial packet failed to send.\n");
		exit(0);
	}
	else{
		printf("Initial packet sent, waiting for ACK.\n");
	}

	// Wait for acknowledgement from the server	
	ret = recvfrom(sock, rec_buffer, MinBuffer, 0, (struct sockaddr *)&server_address, &server_len); 
//	delay(5);
	if(ret == -1){
		printf("Error receiving ACK.\n");
	}
	else{
		printf("ACK for handshake received!\n");
	}
	//Frame ack_hello = malloc(sizeof(Frame));
	//ack_hello = deserialize(rec_buffer);	

	// Send file request packet
	Frame request = malloc(sizeof(Frame));
	request->frame_type = 3;
	request->seq_num = packet_number;
	strcpy(request->file_path, Path);
	packet_number += 1;

	char * file_request = serialize(request);
	char file_req[sizeof(file_request) + 1];
	strcpy(file_req, file_request);	

	count = sendto(sock, file_req, sizeof(file_req), 0, (struct sockaddr *)&server_address, server_len);

	if(count == -1){
		printf("file request packet failed to send.\n");
		exit(0);
	}
	else{
		printf("File request packet sent, waiting for file size.\n");
	}

	//Clear buffer
	memset(rec_buffer, '\0', sizeof(rec_buffer));	

	// Wait for the size of the packet.
	ret = recvfrom(sock, rec_buffer, MinBuffer, 0, (struct sockaddr *)&server_address, &server_len); 
	if(ret == -1){
		printf("Error receiving FileSize.\n");
		exit(0);
	}

	Frame SizeF = malloc(sizeof(Frame));
	SizeF = deserialize(rec_buffer);
	// Check the file size
	printf("The size of the file is: %d\n", SizeF->file_size);

	// Create a buffer to hold the contents
	char The_File[SizeF->file_size + 1];	

	// Send an ACK for the file size
	Frame SizeAck = malloc(sizeof(Frame));
	SizeAck->frame_type = 2;
	SizeAck->seq_num = packet_number;
	packet_number += 1;
	SizeAck->ack = SizeF->seq_num + 1;

	char * SAck = serialize(SizeAck);
	char SA[sizeof(SAck) + 1];
	strcpy(SA, SAck);	

	count = sendto(sock, SA, sizeof(SA), 0, (struct sockaddr *)&server_address, server_len);
	if(count == -1){
		printf("Ack for file size failed to send.\n");
		exit(0);
	}

	// Create packets to use during the loop
	// packet to recieve data
	Frame file_data = malloc(sizeof(Frame));
	// packet to send an ack
	Frame data_ack = malloc(sizeof(Frame));

	// Now loop while receiveing packets until close 
	while(file_count <= SizeF->file_size)
	{
		// Clear buffer and packet info from last loop
		memset(data_buffer, '\0', sizeof(data_buffer));	
		clear_frame(file_data);
		clear_frame(data_ack);		

		// Receive a packet 
		ret = recvfrom(sock, data_buffer, SIZE, 0, (struct sockaddr *)&server_address, &server_len);

		if(ret > 0 )
		{
			// Deserialize
			file_data = deserialize(data_buffer);
			printf("Server packet #%d received.\n", file_data->seq_num);
			
			// Save the contents
			file_count += sizeof(file_data->data);			
			memmove((void *)The_File, (void *)&file_data->data[buffptr], sizeof(SIZE));			

			// Send an ack
			data_ack->frame_type = 2;
			data_ack->seq_num = packet_number;
			packet_number += 1;
			data_ack->ack = file_data->seq_num + 1;
	
			// Serialize
			char * d_ack = serialize(data_ack);
			char dack[sizeof(d_ack) + 1];
			strcpy(dack, d_ack);	

			// Send
			sendto(sock, dack, sizeof(dack), 0, (struct sockaddr *)&server_address, server_len);
		}
		else
		{
			printf("Server packet not received.\n");
		}
	}
	//free some memory
	free(hello);
	free(request);
	free(SizeF);
	free(SizeAck);
	free(file_data);
	free(data_ack);

	shutdown(sock, SHUT_RDWR);
	return 0;
}


// Function to convert a string to an IP address
unsigned int IP_converter(char *string_IP)
{
	// Variables to save the IP address
	int a, b, c, d;
	char array_IP[4];

	// Convert the string to ints
	sscanf(string_IP, "%d.%d.%d.%d", &a, &b, &c, &d);

	// Save the ints in the array
	array_IP[0] = a;
	array_IP[1] = b;
	array_IP[2] = c;
	array_IP[3] = d;

	//return the array in the proper format
	return *(unsigned int *)array_IP;
}
