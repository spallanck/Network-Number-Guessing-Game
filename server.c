/** Name: Sophie Pallanck
 * Date: 10/19/2021
 * Class: CSCI 367
 **/

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define QLEN 8 /* size of request queue */

/*------------------------------------------------------------------------
* Program: server
*
* Purpose: allocate a socket and then repeatedly execute the following:
* (1) wait for the next connection from a client
* (2) fork
* (3) play a number guessing game with client
* (4) close the connection
* (5) go back to step (1)
*
* Syntax: ./server port secret_number
*
* port - protocol port number to use
* secret_number - number between [-128, 128] that the user has to guess
*------------------------------------------------------------------------
*/

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
	int optval = 1; /* boolean value when we set socket option */
	int n = 0; /* number of bytes read from recv */
    int16_t usrGuess; /* number that the user guesses */
    int turnNum = 0; /* keeps track of which turn per round */
	pid_t childpid; 
	// The next six variables are the various responses the server will send to the
	// client.
	char win = 'W';
	char newRnd = 'r';
	char tooLow = 'l';
	char tooLowOB = 'L';
	char tooHigh = 'h';
	char tooHighOB = 'H';

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server server_port secret_number\n");
		exit(EXIT_FAILURE);
	}

    int secretNumber = atoi(argv[2]);
    int hiddenNumber = secretNumber;
	if (secretNumber < -128 || secretNumber > 128) {
		fprintf(stderr,"Error: Secret number is out of bounds. Please enter a secret number between [-128, 128]\n");
		exit(EXIT_FAILURE);
	}
    printf("The secret number is: %d\n", secretNumber);

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	port = atoi(argv[1]); /* convert argument to binary */
	if (port > 0) { /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	

	/* Main server loop - accept and handle requests */
	while (1) {
		alen = sizeof(cad);
		if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
			fprintf(stderr, "Error: Accept failed\n");
			exit(EXIT_FAILURE);
		}
		if ((childpid = fork()) == 0) {
			/* Play a game with a client */
			while ((n = recv(sd2, &usrGuess, sizeof(int16_t), 0)) > 0) {
            	printf("User guessed: %hd\n", usrGuess);
            	if (usrGuess == hiddenNumber && turnNum == 0) {
                	printf("The user won the game!\n");
                	send(sd2, &win, sizeof(win), 0);
					turnNum = -1;
					hiddenNumber = secretNumber;
                	close(sd2);
            	} else if (usrGuess == hiddenNumber && turnNum != 0) {
                	turnNum = -1;
                	printf("The user is starting a new round.\n");
                	send(sd2, &newRnd, sizeof(newRnd), 0);
                	hiddenNumber = secretNumber;
            	} else {
                	int newNum = hiddenNumber + (hiddenNumber - usrGuess);
                	if (usrGuess < hiddenNumber && newNum < 256) {
                    	printf("The user guessed two low.\n");
                    	send(sd2, &tooLow, sizeof(tooLow), 0);
                    	hiddenNumber = newNum;
                	} else if (usrGuess < hiddenNumber && newNum >= 256) {
                    	printf("The user guessed two low and ended the game.\n");
                    	send(sd2, &tooLowOB, sizeof(tooLowOB), 0);
						hiddenNumber = secretNumber;
                    	close(sd2);
						turnNum = -1;
                	} else if (usrGuess > hiddenNumber && newNum > -256) {
                    	printf("The user guessed too high.\n");
                    	send(sd2, &tooHigh, sizeof(tooHigh), 0);
                    	hiddenNumber = newNum;
                	} else {
                    	printf("The user guessed too high and ended the game.\n");
                    	send(sd2, &tooHighOB, sizeof(tooHighOB), 0);
                    	hiddenNumber = secretNumber;
						turnNum = -1;
						close(sd2);
                	}
            	}
            	turnNum++;
            	bzero(&usrGuess, sizeof(usrGuess));	
			}
			close(sd2);
			break;
		/* Check for the case where the fork fails. */
		} else if (childpid < 0) {
			fprintf(stderr,"Forking failed!\n");
			exit(EXIT_FAILURE);
		}
		close(sd2);
	}
		
}

