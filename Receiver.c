
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
#define RECEIVER_ADDR '127.0.0.1'
#define BUFFER_SIZE 8192
#define HALF_FILE 998717
#define ARR_TIME 1000

//func that calculate the time that take to receive half of the file
float time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
}

int main(){

    //opening a socket for the sender
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    //check if there is no exception
    if (sock == -1) {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // used for IPv4 communication
    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));

    recv_addr.sin_family = AF_INET; // Address family, AF_INET unsigned 
    recv_addr.sin_port = htons(RECEIVER_PORT); // receiver port number 
    recv_addr.sin_addr.s_addr  = INADDR_ANY; // Internet address
    int rval = inet_pton(AF_INET,"127.0.0.1", &recv_addr.sin_addr);  // convert IPv4 addresses from text to binary form

    //check if there is no exception
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    } 

    // Bind the socket to the port with any IP
    int bindResult = bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

	//check if there is no exception
    if (bindResult == -1) {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }

    // Make the socket listening
    int listenResult = listen(sock, 2);

	//check if there is no exception
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(sock);
        return -1;
    }

    //Accept and incoming connection
    struct sockaddr_in senderAddress;
    socklen_t senderAddressLen = sizeof(senderAddress);

    memset(&senderAddress, 0, sizeof(senderAddress));
    senderAddressLen = sizeof(senderAddress);
    int senderSocket = accept(sock, (struct sockaddr *)&senderAddress, &senderAddressLen);

    //check if there is no exception
    if (senderSocket == -1) {
        printf("listen failed with error code : %d", errno);
        // close the sockets
        close(sock);
        return -1;
        }

    //define two arrays which will save the times and a counter which will save the number of times     
    float timeOfTheFirstHalf [ARR_TIME] = {0.0};
    float timeOfThesecondHalf [ARR_TIME] = {0.0};
    int countTimes = 0;

    /*save a var that tell if the user want to sent 
    (it will define at the beginning as 1 to enter the first time anyaway)
    and an array that will save the file from the sender*/
    int yes = 1;
    char bufferReply[BUFFER_SIZE];
    while (yes){

        /*receive the first part of the file
        var to enter the saving time only in the first time of the while
        */
        int recvEachTime = 0;
        int bytesReceived = 0;
        int t = 1; 

        //define a struct for the beforetime
        struct timeval before_time1;

        //receive the first part of the file
        while(recvEachTime < HALF_FILE){
            bzero (bufferReply, sizeof(bufferReply));
            bytesReceived = (int)(recv(senderSocket, bufferReply, BUFFER_SIZE, 0));

            //if we receive only one byte it means that it the exit message
            if (bytesReceived == 1){
                break;
            }

            //save the before time
            if (t) {
                gettimeofday(&before_time1, NULL);
                t = 0;
            }

            recvEachTime += bytesReceived;

            //check if there is no exception
            if (bytesReceived == -1) {
                perror("Error: ");
            }
            else if (bytesReceived == 0) {
                printf("peer has closed the TCP connection prior to recv().\n");
            }
        }

        //if we got the exit message
        if (bytesReceived == 1){
                break;
            }

        //save the time after we receive the first part of the file
        struct timeval after_time1;
        gettimeofday(&after_time1, NULL);

        //calculate the time that take to receive half of the file
        timeOfTheFirstHalf[countTimes] = time_diff (&before_time1,&after_time1);

        printf("We got the first message\n");

        //xor between the end of 2 our I_D (3308 xor 8693)
        char auth[] = "10110100011001";

        // Sent an authentication to the sender
        int bytesSent = send(senderSocket, auth, sizeof(auth) , 0);

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
        else {
            printf("auth was successfully sent.\n");
        }
        printf ("Send the authentication\n");

        //change the CC algo
        int CC = setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4);
        if (CC != 0){
            perror("setsockopt");
        }
        printf("Change the algo\n");

        
        recvEachTime = 0;
        t = 1;

        //save the time before we receive the second part of the file
        struct timeval before_time2;

        //receive the second part of the file
        while(recvEachTime < HALF_FILE){

            bzero (bufferReply, sizeof(bufferReply));
            int bytesReceived = (int)(recv(senderSocket, bufferReply, BUFFER_SIZE, 0));

            //save the before time
            if (t){
                gettimeofday(&before_time2, NULL);
                t = 0;
            }
            recvEachTime += bytesReceived;

            //check if there is no exception
            if (bytesReceived == -1) {
                perror("Error: ");
            }
            else if (bytesReceived == 0) {
                printf("peer has closed the TCP connection prior to recv().\n");
            }
        }

        //save the time after we receive the second part of the file
        struct timeval after_time2;
        gettimeofday(&after_time2, NULL);

        printf("Got the second part of the file\n");

        //calculate the time that take to receive half of the file
        timeOfThesecondHalf[countTimes] = time_diff(&before_time2, &after_time2);

        countTimes++;

        //change the CC algo
        CC = setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5);
        if (CC != 0){
            perror("setsockopt");
        }
        printf("Change the algo\n");
    
    }

    //Printing out the times and calaulating the average time
    float aver1 = 0;
    float aver2 = 0;

    for (int i = 0; i < countTimes; i++){
        printf("First part: %f, Second part: %f\n", timeOfTheFirstHalf[i], timeOfThesecondHalf[i]);
        aver1 += timeOfTheFirstHalf[i];
        aver2 += timeOfThesecondHalf[i];
    }
    aver1 = (aver1/countTimes);
    aver2 = (aver2/countTimes);

    printf ("The average times of sent of the first part: %f\n", aver1);
    printf ("The average times of sent of the second part: %f\n", aver2);

    close(senderSocket);
    return 0;

}




