/* Library */
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>

#include "dcomm.h"
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 2500
/* Define receive buffer size */
#define RXQSIZE 8

int parentExit = 0;

/* Limits */
int minUpperLimit = 2; /* To Send XOFF */
int maxLowerLimit = 4; /* To Send XON */ 

/* Socket */
int sockfd; // listen on sock_fd
struct sockaddr_in myAddr;	/* our address */
struct sockaddr_in targetAddr;	/* target address */
unsigned int addrLen = sizeof(targetAddr);	/* length of address */
int targetPort;
int recvLen; /* # byte receive */

Byte buf[1]; /* buffer of buffer */
Byte consumed[1];

char ip[INET_ADDRSTRLEN];

static QTYPE buffer; /* the buffer */
int x = 1; /* 0 = XOFF, 1 = XON */
int receivedByte = 0;
int consumedByte = 0;

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE* queue);
static Byte *q_get(int sockfd, QTYPE* queue);
void* threadParent(void *arg);
void* threadChild(void *arg);

int main(int argc, char *argv[]) {
	Byte c;

	CreateQueue(&buffer, RXQSIZE);

	/* Creating Socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	targetPort = atoi(argv[1]);

	memset((void *)&myAddr, 0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "localhost", &(myAddr.sin_addr));
	myAddr.sin_port = htons(targetPort);

	/* Binding socket pada localhost, 127.0.0.1 */	
	if (bind(sockfd, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
		perror("bind failed");
		return 0;
	}
	
	inet_ntop(AF_INET, &(myAddr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Binding pada 127.0.0.1:%d\n", ntohs(myAddr.sin_port));
	
	/* Mengatur target kepada sender untuk mengirim XON dan XOFF */
	memset((char *) &targetAddr, 0, sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	
	/* Thread initialization */
	pthread_t tid[2];
	int err;

	/* Threading */
	err = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	err = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	/* Joining thread, finishing program */
	pthread_join( tid[0], NULL);
	pthread_join( tid[1], NULL);
 
	/* Closing socket */
	close(sockfd);
	exit(EXIT_SUCCESS);
}

void* threadParent(void *arg) {
	/* Parent Thread */
	Byte *chck;
	
	/* Ketika belum diterima EOF, 
	   teruskan listening untuk penerimaan byte */
	while (parentExit == 0) {	
		chck = rcvchar(sockfd, &buffer);
		if ( chck != NULL ) {
			if ( *chck != 26 ) {
				receivedByte++;
				printf("Menerima byte ke-%d.\n", receivedByte);
			} else {
				printf("EOF diterima\n");
				parentExit = 1;
			}
		}
	}

	char state[1];
	sprintf(state, "%c",(char) XOFF);
	if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
		perror("sendto");
	}
	return NULL;
}

void* threadChild(void *arg) {
	/* Child Thread */
	Byte *chck;

	/* Sampai program diakhiri, konsumsi terus byte yang ada pada buffer*/
	while ((parentExit == 0) || (consumedByte != receivedByte)) {	
		chck = q_get(sockfd, &buffer);
		if ( chck != NULL ) {
			consumedByte++;
			printf("Mengkonsumsi byte ke-%d: '%s'\n", consumedByte, chck);
		}
		usleep(DELAY * 1000);
		
	}

	return NULL;
}

static Byte *rcvchar(int sockfd, QTYPE* queue) {
	/*
	Read a character from socket and put it to the receive buffer.
	If the number of characters in the receive buffer is above certain
	level, then send XOFF and set a flag.
	Return a buffer value.
	*/
	int emptySpace = EmptySpace(*queue);

	if (emptySpace <= minUpperLimit) {
		char state[1];

		printf("Buffer > minimum upperlimit. Mengirim XOFF.\n");
		sprintf(state, "%c",(char) XOFF);
		if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
			perror("sendto");
		}

		x = 0;

		sleep(5);
		
		return NULL;

	} else {

		recvLen = recvfrom(sockfd, buf, 1, 0, (struct sockaddr *)&targetAddr, &addrLen);

		if ( buf[0] != 26 ) {
			Add(queue, buf[0]);
		}	

		return buf;
	}

}

/* q_get returns a pointer to the buffer where data is read or NULL if
 * buffer is empty.
 */
static Byte *q_get(int sockfd, QTYPE *queue) {
	int emptySpace = EmptySpace(*queue);

	if (emptySpace < RXQSIZE) {
		/*
		Retrieve data from buffer
		If the number of characters in the receive buffer is below certain
		level, then send XON.
		Increment front index and check for wraparound.
		*/
		if ( ( emptySpace >= maxLowerLimit ) && ( x == 0 ) ) {
			char state[1];

			printf("Buffer < maximum lowerlimit. Mengirim XON.\n");
			sprintf(state, "%c",(char) XON);
			if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen)==-1) {
				perror("sendto");
			}
			x = 1;

			sleep(5);

			return NULL;

		} else {

			consumed[0] = Del(queue);
			
			while ( !(consumed[0] >= 32 || consumed[0] == 13 || consumed[0] == 10) && EmptySpace(*queue) > 0 ) {
				consumed[0] = Del(queue);
			}

			return consumed;

		}
	} else {
		/* Nothing in the queue */
		return NULL;
	}

}
