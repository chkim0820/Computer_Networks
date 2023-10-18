/**
* @file proj3.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for a simple web server for CSDS 325 HW3
* @date 2023-10-05
*/

#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

using namespace std;

/* Defining macros */
#define ERROR 1
#define INT_ERROR -1
#define CHAR_SIZE 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
int N_ARG = INT_ERROR;
int D_ARG = INT_ERROR;
int A_ARG = INT_ERROR;
string port = "";
string docDirectory = "";
string authToken = "";

/* Methods */

/**
 * @brief Prints out error messages when called; got from sample.c
 * @param format Given format of the message to be printed
 * @param arg Replaced with the placeholder; NULL if none
 */
void errorExit (const char *format, const char *arg) {
    fprintf(stderr, format, arg);
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
        else if (i == N_ARG + 1)
            port = arg;
        else if (i == D_ARG + 1)
            docDirectory = arg;
        else if (i == A_ARG + 1)
            authToken = arg;
        else
            errorExit("Invalid or out-of-order argument %s\r\n\r\n", arg);
    }
    if (N_ARG == INT_ERROR || D_ARG == INT_ERROR || A_ARG == INT_ERROR || // Not all options are present
        port.empty() || docDirectory.empty() || authToken.empty()) // Specifications not all present 
        errorExit("Some or all of the required options are not present\r\n\r\n", nullptr);
}

void sendFile(FILE* file, const int clientSocket) { // FIX: Make sure lines end with \r\n
    char buffer[BUFLEN];
    memset(buffer, 0x0, sizeof(buffer)); 
    size_t lenRead;

    while ((lenRead = fread(buffer, CHAR_SIZE, BUFLEN, file)) > 0) { // check \r\n
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
            errorExit("Error while reading file", nullptr);
    }
    fclose(file);
}

void getMethod(string arg, const int clientSocket) { // arg is the filename & docDirectory/Root is terminal input; also check if input arg valid
    if (arg[0] != '/')
        errorExit("HTTP/1.1 406 Invalid Filename\r\n\r\n", nullptr);
    if (arg == "/")
        arg = "/homepage.html";

    // Check if docDirectory from terminal input is valid
    filesystem::path path(docDirectory);
    if (!filesystem::is_directory(path))
        errorExit("The base directory given by the terminal is incorrect", nullptr);
    
    // Combine file and base directory for a full directory
    string fullDirectory = docDirectory + arg;

    // Opening a file if it exists
    FILE* file = fopen(fullDirectory.c_str(), "rb");
    if (file != nullptr) { // requested file exists
        fprintf(stdout, "HTTP/1.1 200 OK\r\n\r\n");
        sendFile(file, clientSocket);
    }
    else // FIX; decide whether to use errorExit or fprintf
        fprintf(stderr, "HTTP/1.1 404 File Not Found\r\n\r\n");
}

void shutdownMethod(string arg) { // arg used to authenticate client
    if (arg != authToken) // no match
        fprintf(stdout, "HTTP/1.1 403 Operation Forbidden\r\n\r\n");
    else // argument in HTTP request matches -a authToken
        fprintf(stdout, "HTTP/1.1 200 Server Shutting Down\r\n\r\n");
        // continue accepting further connections and requests
}

bool requestLine(string method, string arg, string httpVer, const int clientSocket) {
    // check if "HTTP/"
    if (httpVer.length() != 8 || // ignoring what's after "http/" per the instruction
        (httpVer.length() > 5 && httpVer.substr(0, 5) != "HTTP/"))
        errorExit("HTTP/1.1 400 Malformed Request\r\n\r\n", nullptr);

    if (method == "GET") { // "GET" method requested
        getMethod(arg, clientSocket);
        return true;
    }
    else if (method == "SHUTDOWN") {// "SHUTDOWN" method requested
        shutdownMethod(arg);
        return true;
    }
    else {
        errorExit("HTTP/1.1 405 Unsupported Method\r\n\r\n", nullptr);
        return false;
    }
}

vector<string> httpRequest(const char* buffer, int len, const int clientSocket, bool firstIt) {
    // Parse http request
    std::vector<char> httpResponse;
    httpResponse.insert(httpResponse.begin(), buffer, buffer + len); // append data to httpResponse
    if (httpResponse.empty())
        errorExit("HTTP/1.1 400 Malformed Request\r\n\r\n", nullptr);
    bool rEncountered = false;
    bool firstLine = true;
    vector<string> requestLineElem;
    string word = "";
    for (const char& character : httpResponse) {
        bool isN = (character == '\n');
        bool isR = (character == '\r');
        if (isR)
            rEncountered = true;
        else if ((rEncountered && !isN) || (!rEncountered && isN))
            errorExit("HTTP/1.1 400 Malformed Request\r\n\r\n", nullptr);
        else if (firstIt && firstLine) {
            if (isN || character == ' ') {    
                bool wordEmpty = word.empty();
                if (!wordEmpty)
                    requestLineElem.push_back(word);
                if (wordEmpty || requestLineElem.size() != 3)
                    errorExit("HTTP/1.1 400 Malformed Request\r\n\r\n", nullptr);
                word.clear();
            }
            else
                word = word + character;
        }
        if (!isR)
            rEncountered = false;
    }
    size_t responseSize = httpResponse.size();
    bool emptyEnd = ((responseSize > 1) &&
                     httpResponse[responseSize - 2] == '\r' && httpResponse.back() == '\n');
    bool emptyLineEnd = (responseSize > 2 && (httpResponse[responseSize - 3] == '\n'));
    if (emptyEnd) {
        if (emptyLineEnd == true || responseSize == 2)
            requestLineElem.push_back("TRUE");
        return requestLineElem;
    }
    else
        errorExit("HTTP/1.1 400 Malformed Request\r\n\r\n", nullptr);
}

bool clientSocket(const int clientSocket) {
    bool firstIt = true;
    vector<string> request;
    bool notEmpty = false;
    while (true) {    
        char buffer[BUFLEN];
        memset(buffer, 0x0, sizeof(buffer)); // Clear the buffer
        size_t receivedLen = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (receivedLen < 0)
            errorExit("Error reading from socket", nullptr);
        // Process the HTTP request in the buffer
        vector<string> returnValues = httpRequest(buffer, receivedLen, clientSocket, firstIt);
        size_t vecSize = returnValues.size();
        if (vecSize > 2)
            request = returnValues;
        notEmpty = !request.empty();
        if (vecSize == 4 || (vecSize == 1 && notEmpty))
            break;
        else if (vecSize == 3 || (vecSize == 0 && notEmpty))
            continue;
        else
            errorExit("No request line but already ended", nullptr);
        firstIt = false;
    }
    if (notEmpty)
        return requestLine(request[0], request[1], request[2], clientSocket);
    else
        errorExit("no correct formatting", nullptr);
}

/**
 * @brief Establishes the server; taken from socketsd.c example code
 */
void serverConnect() {
    struct sockaddr_in sin; // Stores info about network endpoint for the server
    struct sockaddr addr;
    struct protoent *protocolInfo; // retrieves network protocols; for TCP
    unsigned int addressLen;
    // int sd, sd2;

    // determine protocol; only proceed if TCP
    if ((protocolInfo = getprotobyname(PROTOCOL)) == nullptr) // Retrieves TCP information
        errorExit("cannot find protocol information for %s", PROTOCOL);

    // setup endpoint urlInfo
    memset((char*)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET; // IPv4 address family
    sin.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    sin.sin_port = htons((u_short)stoi(port)); // Set to the port number specified; converted to big endian

    // allocate a socket; SOCK_STREAM for TCP; would be SOCK_DGRAM for UDP
    int sd = socket(PF_INET, SOCK_STREAM, protocolInfo->p_proto);
    if (sd < 0)
        errorExit("cannot create socket", nullptr);

    // bind the socket; local IP address & port 
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errorExit("cannot bind to port %s", port.c_str());

    // listen for incoming connections; 1 max. pending connections
    if (listen(sd, QLEN) < 0)
        errorExit("cannot listen on port %s\n", port.c_str());
    
    while (true) {
        // accept a connection
        addressLen = sizeof(addr);
        const int sd2 = accept(sd,&addr,&addressLen); // Handles communication with each client
        if (sd2 < 0)
            fprintf(stderr, "error accepting connection");

        // read, write, and send to the connection (sd2)
        bool closeConnection = clientSocket(sd2);
        close(sd2);

        // close connection when "GET" or "SHUTDOWN" methods are called
        if (closeConnection)
            break;
    }

    // close connections and exit
    close(sd);
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