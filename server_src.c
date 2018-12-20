/*select and repeat  functionality */
#include<stdio.h>		//printf
#include<string.h>		//memset
#include<stdlib.h>		//exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include<time.h>
//Add packet size in bytes here
#define DATALEN 20

//The port on which to listen for incoming data
#define PORT 8882




void die (char *s)
{
	perror (s);
	exit (1);
}

// define structures of hello,data and ack packets

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
	long lngth; // length of file in bytes
}HELLO_PKT;




int main (int argc, char *argv[])
{

	float DRATE=0;

	float num=2;

	printf( "Enter Drop Rate between 0 to 100%% :");
	scanf("%f",&DRATE);
	DRATE=100-DRATE;
	DRATE=DRATE/100;

	struct sockaddr_in si_me, si_other;
	int s, i, slen = sizeof (si_other), recv_len;

	DATA_PKT rcv_pkt;
	ACK_PKT ack_pkt;
	HELLO_PKT hello_pkt;
	//create a UDP socket
	if ((s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die ("socket");
	}
	// zero out the structure
	memset ((char *) &si_me, 0, sizeof (si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons (PORT);
	si_me.sin_addr.s_addr = htonl (INADDR_ANY);
	//bind socket to port
	if (bind (s, (struct sockaddr *) &si_me, sizeof (si_me)) == -1)
	{
		die ("bind");
	}

	if ((recv_len =recvfrom (s, &hello_pkt, sizeof(hello_pkt), 0,(struct sockaddr *) &si_other, &slen)) == -1)
	{
		die ("recvfrom()");
	}

	printf("Setting window size to %d packets\n",hello_pkt.window_size);

	const int WINDOW = hello_pkt.window_size;

	const int packets=hello_pkt.total_pkts;

	const long length=hello_pkt.lngth;

	char  buf[length+1];

	fflush(stdout);
	int  rcvd[packets];


	for(int i=0;i<packets;i++)
		rcvd[i]=0;

	int packts_rcvd=0;
	FILE *new = fopen ("out.txt", "w");


	int BASE=0;
	int window_end=BASE+WINDOW-1;
	memset(buf, '\0',length+1 );
	int index=0;
	while(packts_rcvd<packets)
	{
		srand(time(0));
		num = drand48();
		fflush (stdout);
		// main loop for recieving data packets
		if ((recv_len =recvfrom (s, &rcv_pkt, sizeof(rcv_pkt), 0,(struct sockaddr *) &si_other, &slen)) == -1)
		{
			die ("recvfrom()");
		}
		//logic to drop the packet or accept the packet

		if (num<DRATE)
		{

			index=rcv_pkt.sq_no;
			rcvd[index]=1;
			ack_pkt.sq_no = rcv_pkt.sq_no;


			strncpy(buf+(index*(DATALEN-1)),rcv_pkt.data,DATALEN-1);
			if(BASE==rcv_pkt.sq_no)
			{
				//inorder reception of packets
				BASE++;
				window_end++;
				int window_full=1;
				//checking for out of order acknowledgements if any and slide window accordingly
				for(int i=BASE;i<=window_end;i++)
				{
					if(rcvd[i]==0)
					{
						BASE=i;
						window_end=BASE+WINDOW-1;
						window_full=0;
						break;
					}
				}
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
			packts_rcvd++;
			
			//send ack to the client
			if (sendto(s, &ack_pkt, recv_len, 0, (struct sockaddr *) &si_other,slen) == -1)
			{
				die ("sendto()");
			}

			printf("RECIEVE PACKET:%d: ACCEPT : BASE %d\n",rcv_pkt.sq_no,BASE);
			printf("SENT ACK %d \n",ack_pkt.sq_no);



		} 
		else{	//packet dropped 

			printf("RECIEVE PACKET:%d: DROP : BASE %d\n",rcv_pkt.sq_no,BASE);

		}


	}
	fputs (buf, new);
	fclose(new);
	close(s);
	return 0;
}
