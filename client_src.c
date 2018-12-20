#include<stdio.h>		//printf
#include<string.h>		//memset
#include<stdlib.h>		//exit(0);
#include<arpa/inet.h>		//contains the address structures
#include<sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
//The port on which to send data
#define PORT 8882
//change packet size here
#define DATALEN 20 

//declare all the packet structures
typedef struct packet1
{
	int sq_no;
} ACK_PKT;
typedef struct packet2
{
	int sq_no;
	int pkt_size;
	char data[DATALEN];
} DATA_PKT;
typedef struct packet3
{
	int window_size;
	int total_pkts;
	long lngth;
} HELLO_PKT;

void die (char *s)
{
	perror (s);
	exit (1);
}


int main (void)
{
	//logic to read files comes over here
	FILE *f = fopen ("in.txt", "r");
	char *message = 0;
	long length;
	//read the file into a buffer
	if (f)
	{
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		message = (char *) malloc (length);

		int status = 0;
		if (message)
		{
			status = fread (message, 1, length, f);	//read bytes of size 1
		}
		if (status < 0)
		{
			printf ("error exiting");
			exit (1);
		}
		fclose (f);
	}

	// get the window size from the input
	int WINDOW =0;
	printf("Enter Window size:");
	scanf("%d",&WINDOW);

	struct sockaddr_in si_other;
	int s, slen = sizeof (si_other);

	//Calculate the number of packets 
	int packets = length / (DATALEN-1);
	if ((length % (DATALEN-1)) > 0)
		packets++;

	//Calculate number of windows
	int windows = packets / WINDOW;

	if ((packets % WINDOW) > 0)
		windows++;

	int ackd[packets];
	int sent[packets];
	DATA_PKT send_pkt, rcv_ack;
	HELLO_PKT hello_pkt;
	//create a UDP socket
	if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die ("socket");
	}
	int on =1;
	int rc=ioctl(s, FIONBIO, (char *)&on);
	if(rc<0)
		printf("ioctl failed");

	memset ((char *) &si_other, 0, sizeof (si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons (PORT);
	si_other.sin_addr.s_addr = inet_addr ("127.0.0.1");

	printf ("Sending hello packet with window size as:%d\n", WINDOW);

	hello_pkt.window_size = WINDOW;
	hello_pkt.total_pkts = packets;
	hello_pkt.lngth=length;
	//send hello packet to confirm window size with server
	if (sendto(s, &hello_pkt, sizeof (hello_pkt), 0, (struct sockaddr *) &si_other,slen) == -1)
	{
		die ("sendto()");
	}

	for(int i=0;i<packets;i++)
	{
		ackd[i]=0;
		sent[i]=0;
	}
	int packts_sent = 0;
	int packets_ackd=0;
	int BASE=0;

	int window_end= BASE+WINDOW-1;

	for (int i = 0; i < WINDOW; i++)
	{

		sendPacket(s,&send_pkt,sizeof (send_pkt),message,packts_sent,&si_other,slen,0);
				sent[i]=1;


				printf("BASE %d\n",BASE);
				packts_sent++;

				}

				while (packets_ackd < packets)
				{

				//block for receiving acknowledgements
				int retransmit =0;
				struct timeval Timeout;
				Timeout.tv_sec = 2;       /* timeout (secs.) */
				Timeout.tv_usec = 0;            /* 0 microseconds */
				fd_set readSet;
				FD_ZERO(&readSet);
				FD_SET(s,&readSet);
				//select is used for timer purposes
				int Ready = select(s+1, &readSet, NULL, NULL, &Timeout);

				if(Ready>0)
				{

					if (recvfrom (s, &rcv_ack, sizeof (rcv_ack), 0, (struct sockaddr *) &si_other, &slen) == -1)
					{
						die ("recvfrom()");
					}

					fflush(stdout);
					ackd[(rcv_ack.sq_no)]=1;

					//checking for inorder packets
					if(BASE==(rcv_ack.sq_no))
					{
						if(window_end>packets)
							window_end=packets-1;
						int window_full=1;
						//check packets in the window that have already been acked,slide window accordingly.
						for(int i=BASE; i<=window_end;i++)
						{

							if(ackd[i]==0)
							{
								BASE=i;
								window_end=BASE+WINDOW-1;
								if(window_end>packets)
									window_end=packets-1;
								window_full=0;
								break;
							}

						}


						//if window full then directly slide it
						if(window_full)
						{
							BASE=window_end+1;
							window_end=BASE+WINDOW-1;
							if(window_end>packets)
								window_end=packets;
							if(BASE>packets)
								BASE=packets;


						}
					}
					printf ("RECIEVED ACK  %d :BASE %d\n ", rcv_ack.sq_no,BASE);
					//checking for packets that have not been sent in the current window ; send if not sent
					for(int i=BASE;i<=window_end;i++)
					{

						if(sent[i]==0 && window_end<packets){
							sendPacket(s,&send_pkt,sizeof (send_pkt),message,i,&si_other,slen,retransmit);
							sent[i]=1;
							printf (" BASE %d \n", BASE);

						}
					}

					packets_ackd++;


				}



				else{
					//handles timeout condition i.e. resends timedout packets

					printf("TIMEOUT %d\n",BASE);

					retransmit=1;

					sendPacket(s,&send_pkt,sizeof (send_pkt),message,BASE,&si_other,slen,retransmit);
				}

				}

				close (s);
				return 0;
}

/* utility function to send packets to the server
s: Socket FD returned while creating the conn.
DATA_PKT : the data to be sent
packet_size:size of data in the packet
message: data from  the file to be sent
packts_sent: packet to be sent
si_other:socket structure of the server
retransmit: 1 if retransmission 0 otherwise
 */
void sendPacket(int s, DATA_PKT * send_pkt,int packet_size, char * message, int packts_sent, struct sockaddr_in * si_other,int slen,int retransmit)
{
	memset( send_pkt->data, '\0', sizeof( send_pkt->data));
	strncpy (send_pkt->data,
			message + (packts_sent*(DATALEN -1)),
			DATALEN - 1);
	printf ("SEND packet %d : ", packts_sent);
	if(retransmit)
		printf ("BASE %d \n", packts_sent);


	send_pkt->sq_no = packts_sent;
	send_pkt->pkt_size = DATALEN;


	if (sendto(s, send_pkt, packet_size, 0,(struct sockaddr *) si_other, slen) == -1)
	{
		die ("sendto()");
	}


}




