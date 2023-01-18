
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define RECEIVER_PORT 10000
#define BUFFER_SIZE 1000000

int main(){

    //read the file
	FILE *f = fopen ("file.txt","r");
    char ch = '0';
	int sizefile = 0;
 
    //count the length of the file
    while((ch=getc(f))!=EOF){
        sizefile++;
    }
    int c = (sizefile/2);

    //define 2 arrays for the first and second parts of the file
    char arr1[c];
    char arr2[c];

    fclose(f);

    FILE *f1 = fopen ("file.txt","r");
    //insert the file text into the arrays
    for(int i = 0; i<(sizefile); i++){

        if (i<c){
            arr1[i] = getc(f1);
        }
        else{
            arr2[i-c] = getc(f1);
        } 
    }
    fclose(f1);
    
    //opening a socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	//check if there is no exception
	if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return 1;
    }

	// used for IPv4 communication
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET; // Address family, AF_INET unsigned 
	serv_addr.sin_port = htons(RECEIVER_PORT); // Port number  
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Internet address

    // Make a connection to the receiver with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    //check if there is no exception
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock);
        return -1;
    }

    //the while will run till the user sent him N to exit
    int yes = 1;
    while (yes) {

        // sent the first part of the file
        int bytesSent = send(sock,arr1 ,c, 0);

        //check if there is no exception
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
            perror("Error: ");
            close(sock);
            return -1;
        } 
        else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        }
        printf ("Send the first part\n");

        //xor between the end of 2 our I_D (3308 xor 8693)
        char auth[] = "10110100011001";

        //receive auth from receiver
        char bufferReply[sizeof(auth)] = {'\0'};
        int bytesReceived = recv(sock,bufferReply, sizeof(bufferReply), 0);

        //check if the auth is OK
        int authEqual = 1;
        for (int i = 0 ; i< sizeof(auth); i++){
            if (auth[i] != bufferReply[i]){
                authEqual = 0;
                printf("\nThe authentication is wrong, the connection closed.");
                close(sock);
                break;
            }
        }
        printf("Got the authentication\n");

        //if the auth is OK we will change the CC algo and continue
        if (authEqual) {
            //change the CC algo
            int CC = setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4);
            if (CC != 0){
                perror("setsockopt");
            }

            printf ("Changed the algo\n");
 
            //sent the second part of the file
            int bytesSent = send(sock,arr2 ,c, 0);

            //check if there is no exception
            if (bytesSent == -1) {
                printf("send() failed with error code : %d", errno);
                perror("\nError: ");
                close(sock);
                return -1;
            }
            else if (bytesSent == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            }
        }
        printf ("Send the second part of file\n");

        //ask the user an exit message 
        char Exit = ' ';
        printf ("\nEnter Y if you want to sent the file again or N to exit: ");
        scanf (" %c",&Exit);

        // sent an exit message to the Receiver
        if (Exit == 'N'){
            int exit = send (sock, &Exit, 1, 0);

            //check if there is no exception
            if (exit == -1) {
                printf("send() failed with error code : %d", errno);
                perror("Error: ");
                close(sock);
                return -1;
            } 
            else if (exit == 0) {
                printf("peer has closed the TCP connection prior to send().\n");
            }
            //if the user enter N to exit we will exit the while
            yes = 0;
        }

        printf ("exit or not: %c\n", Exit);

        //if the user want to continue we will change the CC algo to cubic again
        if (yes){
            
            //change the CC algo
            int CC = setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5);
            if (CC != 0){
                perror("setsockopt");
            }
            printf ("Changed the algo\n");
        }
      
    }
    close(sock);
	return 0;

}




