/** Name: Sophie Pallanck
 * Date: 10/19/2021
 * Class: CSCI 367
 **/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*------------------------------------------------------------------------
* Program: client
*
* Purpose: allocate a socket, connect to a server, and play a 
* number guessing game.
*
* Syntax: ./client server_address server_port
*
* server_address - name of a computer on which server is executing
* server_port    - protocol port number server is using
*
*------------------------------------------------------------------------
*/
int main( int argc, char **argv) {
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
    int roundNum = 0; /* keeps track of which round it is */
    int turnNum = 0; /* keeps track of which turn it is per round */
    int won = 0; /* set to zero if the user has not won, and set to 1 when the user wins*/
    int16_t guess; /* stores the user's guess to send to the server */
	char server; /* stores the server's response to the user guess */

	memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./client server_address server_port\n");
		exit(EXIT_FAILURE);
	}

    

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short)port);
	else {
		fprintf(stderr,"Error: bad port number %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}
	

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}
	
	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"connect failed\n");
		exit(EXIT_FAILURE);
	}

	/* Keep playing the game until the user wins*/
    while (won == 0) {

		/* Take a guess from the user and verify it is within bounds before sending to server. */
        printf("Round %d, turn %d\n", roundNum, turnNum);
        printf("Enter guess: "); 
        scanf("%hd", &guess);
		while (guess >= 256 || guess <= -256) {
			printf("Guess must be between [-255, 255]. Please try again.\n");
			printf("Enter guess: "); 
        	scanf("%hd", &guess);
		}
        send(sd, &guess, sizeof(int16_t), 0);
		n = recv(sd, &server, sizeof(server), 0);

        if (server == 'W') {
            printf("You win!\n");
            won = 1;
            close(sd);
        } else if (server == 'r') {
            roundNum++;
            turnNum = 0;
            printf("Correct, advance to the next round.\n");
        } else if (server == 'l') {
            turnNum++;
            printf("Too low, guess again.\n");
        } else if (server == 'L') {
            printf("Too low, and pushed hidden number out of bounds. You lose...\n");
            close(sd);
	        exit(EXIT_SUCCESS);
        } else if (server == 'h') {
            turnNum++;
            printf("Too high, guess again.\n");
        } else if (server == 'H') {
            printf("Too high, and pushed hidden number out of bounds. You lose...\n");
            close(sd);
	        exit(EXIT_SUCCESS);
        }
    }
	close(sd);

	exit(EXIT_SUCCESS);
}

