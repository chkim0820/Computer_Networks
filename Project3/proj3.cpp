/**
* @file proj3.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for a simple web server for CSDS 325 HW3
* @date 2023-10-05
*/

#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

/* Defining macros */
#define ERROR 1
#define INT_ERROR -1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
int N_ARG = -1;
int D_ARG = -1;
int A_ARG = -1;
string port = nullptr;
string docDirectory = nullptr;
string authToken = nullptr;

/* Methods */

/**
 * @brief Prints out error messages when called; got from sample.c
 * @param format Given format of the message to be printed
 * @param arg Replaced with the placeholder; NULL if none
 */
void errorExit (const char *format, const char *arg) {
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}

/**
 * @brief For parsing the input arguments 
 * @param argc Number of arguments
 * @param argv Arguments
 */
void parseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (COMPARE_ARG(arg, "-n"))
            N_ARG = i;
        else if (COMPARE_ARG(arg, "-d"))
            D_ARG = i;
        else if (COMPARE_ARG(arg, "-a"))
            A_ARG = i;
        else if (i == N_ARG + 1) { // FIX: check if these inputs are erroneous later & return error messages
            // port = new char[strlen(arg)];
            // strcpy(port, arg);
            port = arg;
        }
        else if (i == D_ARG + 1) {
            // docDirectory = new char[strlen(arg)];
            // strcpy(docDirectory, arg);
            docDirectory = arg;
        }
        else if (i == A_ARG + 1) {
            // authToken = new char[strlen(arg)];
            // strcpy(authToken, arg);
            authToken = arg;
        }
        else
            errorExit("Invalid argument %s", arg);
    }
    if (N_ARG == INT_ERROR || D_ARG == INT_ERROR || A_ARG == INT_ERROR) // Not all options are present
        errorExit("Some or all of the required options are not present", NULL);
    if (port.empty() || docDirectory.empty() || authToken.empty()) // Specifications not all present
        errorExit("Some or all of the required specifications are not present", NULL);
}

void httpRequest(int clientSocket) {
    char request[BUFLEN];
    char buffer[BUFLEN];
    char method[10]; // FIX; arg length
    char arg[10];
    char ver[10];
    recv(clientSocket, request, BUFLEN, 0); // FIX: longer requests?
    sscanf(request, "%s %s HTTP/", method, arg);
    docDirectory = docDirectory + arg;

    int file = open(docDirectory.c_str(), O_RDONLY);
    if (file == -1)
        ;// FIX: error message
    else {
        int readLen;
        while ((readLen = read(file, buffer, BUFLEN)) > 0)
            write(clientSocket, buffer, readLen);
        if (readLen < 0)
            ; // FIX: error message
    }
}

void clientSocket(int clientSocket) {
    httpRequest(clientSocket);
    char buffer[BUFLEN];
    int readLen;

    if (write(clientSocket, buffer, readLen) < 0)
        errorExit("Error writing message: %s", docDirectory.c_str());

}

/**
 * @brief Establishes the server; taken from socketsd.c example code
 */
void serverConnect() {
    struct sockaddr_in sin; // Stores info about network endpoint for the server
    struct sockaddr addr;
    struct protoent *protocolInfo; // retrieves network protocols; for TCP
    unsigned int addressLen;
    int sd, sd2;

    /* determine protocol; only proceed if TCP */
    if ((protocolInfo = getprotobyname(PROTOCOL)) == NULL) // Retrieves TCP information
        errorExit("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint urlInfo */
    memset((char*)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET; // IPv4 address family
    sin.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    sin.sin_port = htons((u_short)stoi(port)); // Set to the port number specified; converted to big endian

    /* allocate a socket */
    /* SOCK_STREAM for TCP; would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protocolInfo->p_proto);
    if (sd < 0)
        errorExit("cannot create socket", NULL);

    /* bind the socket; local IP address & port */
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errorExit("cannot bind to port %s", port.c_str());

    /* listen for incoming connections; 1 max. pending connections */
    if (listen(sd, QLEN) < 0)
        errorExit("cannot listen on port %s\n", port.c_str());
    
    /* accept a connection */
    addressLen = sizeof(addr);
    sd2 = accept(sd,&addr,&addressLen); // Handles communication with each client
    if (sd2 < 0)
        errorExit ("error accepting connection", NULL);

    /* read & write message to the connection (sd2) */
    clientSocket(sd2);

    /* close connections and exit */
    close(sd);
    close(sd2);
}

/**
* @brief Main method for processing the inputs and running appropriate commands
* @param argc Number of arguments
* @param argv Arguments
* @return int Main method return value
*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv); // When returned, all required inputs present (at least number-wise)

    serverConnect();
}