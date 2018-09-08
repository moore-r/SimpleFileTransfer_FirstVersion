// Ryan Moore, CS494
// File to implement functions from header file


#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>

#include "RM_RCP_H.h"

// Timer function, for timeouts and such
int delay(int num_secs)
{
	time_t start, end;
	double passed;
	int stop = 0;
	start = time(NULL);

	while(stop != -1)
	{
		end = time(NULL);
		passed = difftime(end, start);
		if(passed > num_secs){
			stop = -1;
			return -1;
		}
	}

	return 0;
}


// Serialize the data, take a struct return char array
// Copy the data into the buffer with a series of 
// if statements depending on the frame type
char *serialize(Frame to_send)
{
	char sbuffer[SIZE];
	memset(sbuffer, '\0', SIZE);
	char dbuffer[MinBuffer];
	int off = 0;

	// Every type of packet will have a frame type and a sequence
	// Number so put the in out side of the if statements.
	int x = to_send->frame_type;
	unsigned int y = to_send->seq_num;
	char ty[sizeof(int)];
	sprintf(ty, "%d", x);
	strcat(sbuffer, ty);
	strcat(sbuffer, ";");
	char h[sizeof(unsigned int)];
	sprintf(h, "%d", y);
	strcat(sbuffer, h);
	strcat(sbuffer, ";");


	// Serialize the first connection packet
	// This one has just a sequence number
	if(to_send->frame_type == 1)
	{
		// Dont have to do anything here
	
		/*
		memcpy(sbuffer, &to_send->frame_type, sizeof(int));
		off += sizeof(int);
		memcpy(sbuffer + off, &to_send->seq_num, sizeof(unsigned int));
		*/
	}
	// Serialize an ACK packet
	else if(to_send->frame_type == 2)
	{
		unsigned int r = to_send->ack;
		char q[sizeof(unsigned int) + 1];
		sprintf(q, "%d", r);
		strcat(sbuffer, q);
		strcat(sbuffer, ";");

		/*
		memcpy(sbuffer, &to_send->frame_type, sizeof(int));
		off += sizeof(int);
		memcpy(sbuffer + off, &to_send->ack, sizeof(unsigned int));
		*/
	}
	// Serialize a data request packet
	else if(to_send->frame_type == 3)
	{
		char r[sizeof(to_send->file_path) + 1];
		strcpy(r, to_send->file_path);
		strcat(sbuffer, r);
		strcat(sbuffer, ";");

		/*
		memcpy(sbuffer, &to_send->frame_type, sizeof(int));
		off += sizeof(int);
		memcpy(sbuffer + off, &to_send->file_path, sizeof(to_send->file_path));
		*/
	}
	// Serialize a packet with filesize
	else if(to_send->frame_type == 4)
	{
		unsigned int r = to_send->file_size;
		char q[sizeof(unsigned int) + 1];
		sprintf(q, "%d", r);
		strcat(sbuffer, q);
		strcat(sbuffer, ";");

		/*
		memcpy(sbuffer, &to_send->frame_type, sizeof(int));
		off += sizeof(int);
		memcpy(sbuffer + off, &to_send->ack, sizeof(unsigned int));
		off += sizeof(unsigned int);
		memcpy(sbuffer + off, &to_send->file_size, sizeof(unsigned int));
		*/
	}
	// Serialize the packet that has the actual file data in it
	else
	{
		// Make sure the frame type is correct
		if(to_send->frame_type != 5)
		{
			printf("(Serializing) The frame type is invalid! Not sure what kind of packet this is.\n");
			return 0;
		}

		/*
		char r[sizeof(to_send->data) + 1];
		strcpy(r, to_send->data);
		strcat(sbuffer, r);
		*/

		off = sizeof(char)*2 + sizeof(int) + sizeof(unsigned int);	
		memcpy((void *)dbuffer + off, (void*)&to_send->frame_type, sizeof(MinBuffer));
	}

	// Copy the buffer ito a char * ptr to be returned. 
	char buf[sizeof(sbuffer) + 1];
	strcpy(buf, sbuffer);
	char *ret = buf;

	// Print the contents of the buffer, just a test.
//	printf("Contents of the buffer to be sent are:\n %s\n", ret);
	
	// If everything worked out, return the buffer to be sent
	return ret;
}


// Deserialize the data, take a char array and return a struct
//Use the semi colons put in by the serializer function 
// to break the array into peices 
Frame deserialize(char *received)
{
	Frame ret = malloc(sizeof(Frame));
	int count = -1;
	int asint, frame_value;
	char name_buf[100];
	char record_uint[4];
	int buf_count = 0;
	char full_buf[1024];

	char desbuf[sizeof(received)];
	strcpy(desbuf, received);

	// Check if the asccii value is ;
	while(desbuf[count] != 59)
	{		
		count += 1;
	}
	char des[2];
	des[0] = desbuf[count-1];
	frame_value = atoi(des);
	
	// Store the frame value
	ret->frame_type = frame_value;

	// Get the sequence number;
	count += 1;
	char seq[4];	// Array to hold values of the sequence number
	int seq_count = 0;
	while(desbuf[count] != 59)
	{
		seq[seq_count] = desbuf[count];
		seq_count += 1;
		count += 1;
	}
	asint = atoi(seq);

	// Store the data in the struct
	ret->seq_num = asint; 

	count += 1;
	// Deserialize an ACK
	if(frame_value == 1)
	{
		//do nothing
	}
	else if(frame_value == 2)
	{
		while(desbuf[count] != 59)
		{
			record_uint[buf_count] = desbuf[count];
			buf_count += 1;
			count += 1;
		}
		asint = atoi(record_uint);
		// Store the data in the struct
		ret->ack = asint; 
	}
	// Deserialize a file request packet
	else if(frame_value == 3)
	{
		while(desbuf[count] != 59)
		{
			name_buf[buf_count] = desbuf[count];
			buf_count += 1;
			count += 1;
		}
		// Store the data in the struct
		strcpy(ret->file_path, name_buf); 
	}
	// Deserialize a file size packet
	else if(frame_value == 4)
	{
		while(desbuf[count] != 59)
		{
			record_uint[buf_count] = desbuf[count];
			buf_count += 1;
			count += 1;
		}
		asint = atoi(record_uint);
		// Store the data in the struct
		ret->file_size = asint; 
	}
	// Deserialize a normal packet with data
	else
	{
		if(frame_value != 5)
		{
			printf("(Deserializing) The frame type is invalid! Not sure what kind of packet this is.\n");
			return 0;
		}

		while(buf_count < 1023)
		{
			full_buf[buf_count] = desbuf[count];
			buf_count += 1;
			count += 1;
		}
		// Store the data in the struct
		strcpy(ret->data, full_buf); 
	}

	return ret;
}


// Clear the data in a packet so that we can loop 
// using the same packet.
void clear_frame(Frame clear)
{
	clear->frame_type = '\0';
	clear->seq_num = '\0'; 
	clear->ack = '\0';
	clear->file_size = '\0';
	memset(clear->file_path, '\0', sizeof(clear->file_path));
	memset(clear->data, '\0', sizeof(clear->data));

	return;
}

// Main here is just for testing
/*
int main(){
	// Timer test
//	int timerh = delay(5);
	printf("main.\n");

	// Serialize test
	Frame test = malloc(sizeof(Frame));
	test->frame_type = 5;
	test->seq_num = 12; 
	test->ack = 15;
	test->file_size = 314;
	strcpy(test->file_path, "/home/docs/test");
	strcpy(test->data, "What did you just call m");

	
	char *test_buf = serialize(test);
	printf("Contents of the buffer to be sent are (returned version):\n %s\n", test_buf);


	// Deserialize test
	char *hi;
	int hu = 2;
	if(hu == 1){strcpy(hi, "1;123;"); }
	else if(hu == 2){strcpy(hi, "2;12;15;"); }
	else if(hu == 3){strcpy(hi, "3;12;/home/dops;"); }
	else if(hu == 4){strcpy(hi, "4;12;314;"); }
	else if(hu == 5){strcpy(hi, "5;12;whoareyou"); }

	Frame testd;
	testd = deserialize(hi);
	
	printf("Deserializing info that was returned in struct form: \n");
	printf("Type: %d\n", testd->frame_type);
	printf("seq num: %d\n", testd->seq_num);
	printf("ack: %d\n", testd->ack);
	printf("file size: %d\n", testd->file_size);
	printf("file path: %s\n", testd->file_path);
	printf("data: %s\n", testd->data);


	return 0;
}
*/

