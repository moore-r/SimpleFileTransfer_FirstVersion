// Ryan Moore, CS494, PP!
/* 
	Header file To define some constants, including buffer size
	and create packets. The packets will be as simple as possible.
*/

#define SIZE 2048
// A max and min buffer size
#define MaxBuffer 65536
#define MinBuffer 1024

// A timeout variable, for timeout in seconds
// Will change to miliseconds once program is working
#define TIMEOUT 5


// Packet for intial connections and acks
typedef struct{
	unsigned int packet_num;	
}ack;

// Packet for file request and file size
// The file size field will be blank upon
// file request by client
typedef struct{
	unsigned int packet_num, file_size, name_len;
	char * file_path;	
}request;

// Packet for transmitting the file itself.
typedef struct{
	unsigned int packet_num;
	
}packet;

// The packet structure I plan to use
// Types:
// 1 - connection
// 2 - ACK packet
// 3 - file request packet
// 4 - file size packet
// 5 - file data packet
typedef struct{
	int frame_type;
	unsigned int seq_num;
	unsigned int ack;
	unsigned int file_size;
	char file_path[MinBuffer];
	char data[MinBuffer];
} *Frame;

// A timer function
int delay(int num_secs);
char *serialize(Frame);
Frame deserialize(char *);
void clear_frame(Frame);
