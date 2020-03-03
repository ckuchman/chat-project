/*
Programmer Name: Chris Kuchman
Program Name: Chat Client
Program Description: This program starts a socket connection the the specified host and port. When connection has succeeded, the user is prompted for a 
handle for their messages. They then proceed to send then receive messages from the server (always taking turns with the client first) until one of them 
inputs a \quit prompt, the client then closes the connection and the program ends.
Course Name: CS 372-400
Last Modified: 2/7/2020
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once 
#define MAXMSGSIZE 512  //Max size of message from/or to client
#define MAXNAME 11 //Max size of handle name including null-term


/*
Referenced from https://beej.us/guide/bgnet/examples/client.c
Gets sockaddr, IPv4 or IPv6 dependent on what address is passed in
Pre-conditions:
Passed address must be in a valid form
Post-conditions:
None
Return:
The appropriate address stored in the passed socket address
*/
void *get_in_addr(struct sockaddr *sa){

    //Checks the type of socket passed
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/*
This function takes in a hostname and port and creates a socket for it. The socket file
description is then returned.
Referenced from https://beej.us/guide/bgnet/examples/client.c
Pre-conditions:
The passed in host and port need to be in a valid format
host can be in either IPv4 or IPv6 format
Post-conditions:
None
Return:
The socket file descriptor if successful, 1 if the host/port format is incorrect and 2 if no connection could be made.
*/
int initContact(char *host, char *port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    //Specifies what kind of socket is desired
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        //Either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP connection

    //Construct the addrinfo struct for the specified host and port
    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    //Loop through each socket option result until one allows a connection or all are checked
    for(p = servinfo; p != NULL; p = p->ai_next) {

        //Generate a socket using the values from getaddrinfo
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        //Attempt to connect to the specified socket
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    //If no connection can be made return a error
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    //Converts the host name string to the binary equivalent
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    //Free up the created linked list
    freeaddrinfo(servinfo);

    return sockfd;
}


/*
This function prompts the user for a message to send over the provided socket and adds the users
supplied handle to the start of the message. If the message is \quit then then message is still sent, but
the function returns a boolean to indicate that the user wants to close the connection.
Pre-conditions:
There is a socket connection open
The handle does not exceed 12 characters (including null-term)
Post-conditions:
Message is sent over the socket
Return:
Boolean that indicates if the user wants to close the connection
*/
bool sendMsg(int socket, char* hdl) {
    char cbuf[MAXMSGSIZE], tempbuf[MAXMSGSIZE]; 
    bool quit = false;

    //Client user inputs message to send to server or /quit
    memset(tempbuf, '\0', MAXMSGSIZE);
    printf("%s", hdl);
    fgets(tempbuf, MAXMSGSIZE - 12, stdin);

    //Appends the handle to the start of the string
    memset(cbuf, '\0', MAXMSGSIZE);
    strcpy(cbuf, hdl);
    strcat(cbuf, tempbuf);

    //The message is sent to the server
    if (send(socket, cbuf, MAXMSGSIZE, 0) == -1) {
        perror("send");
    }

    //If the user indicates a quit command, return true for quit
    if (strcmp(tempbuf, "\\quit\n") == 0) { 
        quit = true;
    }
 
    return quit;
}


/*
This function takes in a socket, prompts the user for a message and sends it over the socket. If the user
inputs \quit then the message is still sent, but the function returns a boolean indicating a request to end the connection
Pre-conditions:
There is a socket connection open
Post-conditions:
The input message is sent over the socket
Return:
Boolean that indicates if the user wants to close the connection
*/
bool receiveMsg(int socket) { 
    int numbytes;
    char sbuf[MAXMSGSIZE], tempbuf[MAXMSGSIZE]; 
    bool quit = false;

    //Server sends back message
    memset(sbuf, '\0', MAXMSGSIZE);
    if ((numbytes = recv(socket, sbuf, MAXMSGSIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
 
    //Ensures that the message is null-terminated   
    sbuf[numbytes] = '\0';

    //Checks if /quit was passed by server
    if(strcmp((strchr(sbuf, '>')+1), "\\quit") != 0){
        //Prints the servers message to the screen
        printf("%s\n", sbuf);
    } else {
        printf("Server has closes connection\n");
        quit = true;
    }

    return quit;
}


/*
Main function that starts a socket connection the the specified host and port. When connection has succeeded, the user is prompted for a 
handle for their messages. They then proceed to send then receive messages from the server (always taking turns with the client first) until one of them 
inputs a \quit prompt, the client then closes the connection and the program ends.
Referenced from https://beej.us/guide/bgnet/examples/client.c
Pre-conditions:
Valid host and port is passed in as arguments
Post-conditions:
Messages are sent over to the server till one side ends the connection
Return:
Error codes
*/
int main(int argc, char *argv[])
{
    int sockfd;  
    char handle[MAXNAME+1];
    bool quit = false;

    //Checks if the right number of arguements are passed
    if (argc != 3) {
        fprintf(stderr,"Missing arguments, Required: Hostname Port\n");
        exit(1);
    }

    //Gets for the user a handle to use for messages
    printf("What do you want as your handle? (Max of 10 characters)");
    fgets(handle, 11, stdin);
    printf("You chose %s", handle);

    //Removes the new line ref:https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
    handle[strcspn(handle, "\n")] = 0;

    //Makes handle> format string
    strcat(handle, ">");

    //Creates the socket to the server
    sockfd = initContact(argv[1], argv[2]);   

    //loop that maintains connection until the command /quit is input
    while(!quit) {

        //Sends message to server
        quit = sendMsg(sockfd, handle);

        if (!quit) {
            //Gets message back from server
            quit = receiveMsg(sockfd);

        }
    }

    //Closes socket
    close(sockfd);

    return 0;
}
