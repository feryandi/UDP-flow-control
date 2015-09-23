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
/* Delay to adjust speed of sending bytes */
#define DELAY 1000

/* Socket */
int sockfd; // listen on sock_fd
struct sockaddr_in targetAddr;	/* target address */
unsigned int addrLen = sizeof(targetAddr);	/* length of address */
int targetPort;
int recvLen; /* # byte receive */

Byte buf[1]; /* buffer of buffer */
char bufc[1]; /* buffer of char */

char ip[INET_ADDRSTRLEN];

int x = 1; /* 0 = XOFF, 1 = XON */
int parentExit = 0;
char* filename;

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *);
void* threadParent(void *arg);
void* threadChild(void *arg);

int main(int argc, char *argv[]) {
	Byte c;

	/* Creating Socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	filename = argv[3];
	targetPort = atoi(argv[2]);

	/* Mengatur target pengiriman ke receiver */
	memset((char *) &targetAddr, 0, sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	if (inet_aton((argv[1]), &targetAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	
	inet_ntop(AF_INET, &(targetAddr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Membuat socket untuk koneksi ke %s:%d ...\n", ip, ntohs(targetAddr.sin_port));

	/* Thread initialization */
	pthread_t tid[2];
	int err;

	/* Threading */
	err = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	err = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	/* Thread joining, finishing program */
	pthread_join( tid[0], NULL);
	pthread_join( tid[1], NULL);

	close(sockfd); 
	exit(EXIT_SUCCESS);

}

void* threadParent(void *arg) {
	/* Parent Thread */

	FILE *fp;
	int c, counter = 0;
	fp = fopen(filename, "r");

	/* Read the file, parse and send it as Byte packet */
	while ( ((c = fgetc(fp)) != EOF)) {
		while ( x != 1 ) { 
			/* Busy waiting */ 
			printf("Menunggu XON...\n");
			sleep(1);		
		} 

		counter++;
		printf("Mengirim byte ke-%d: '%c'\n", counter, (char) c);
		sprintf(bufc, "%c", (char) c);
		if (sendto(sockfd, bufc, 1, 0, (struct sockaddr *)&targetAddr, addrLen)==-1) {
			printf("err: sendto\n");	
		}

		usleep(DELAY * 1000);
	}
	
	/* End of file */
	printf("Mengirim EOF\n");
	sprintf(bufc, "%c", (char) 26);
	if (sendto(sockfd, bufc, 1, 0, (struct sockaddr *)&targetAddr, addrLen)==-1) {
		printf("err: sendto\n");	
	}

	//printf("Exiting Parent\n");
	parentExit = 1;

	return NULL;
}

void* threadChild(void *arg) {
	/* Child Thread */

	while (parentExit == 0) {
		/* Listening XON XOFF signal */ 
		char state[1];

		if ( recvfrom(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, &addrLen) == -1 )
			perror("sendto");

		if ( state[0] == XON ) {
			printf("XON diterima.\n"); 
			x = 1;
		} else if ( state[0] == XOFF ) {
			printf("XOFF diterima.\n"); 
			x = 0;
		}
	}

	printf("Exiting Child\n");
	return NULL;
}

