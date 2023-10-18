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
    fprintf (stderr, "\r\n");
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
            errorExit("Invalid or out-of-order argument %s", arg);
    }
    if (N_ARG == INT_ERROR || D_ARG == INT_ERROR || A_ARG == INT_ERROR || // Not all options are present
        port.empty() || docDirectory.empty() || authToken.empty()) // Specifications not all present 
        errorExit("Some or all of the required options are not present", nullptr);
}

void writeToSocket(const int clientSocket, string response) {
    string retResponse = "HTTP/1.1 " + response + "\r\n\r\n";
    char buffer[retResponse.length() + 1];
    memset(buffer, 0x0, sizeof(buffer));
    strcpy(buffer, retResponse.c_str());
    if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
        errorExit("Error while sending response to the socket", NULL);
}

bool sendFile(FILE* file, const int clientSocket) { // FIX: Make sure lines end with \r\n
    char buffer[BUFLEN];
    memset(buffer, 0x0, sizeof(buffer)); 
    size_t lenRead;

    while ((lenRead = fread(buffer, CHAR_SIZE, BUFLEN, file)) > 0) { // check \r\n
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
            errorExit("Error while reading the input file", nullptr);
    }
    fclose(file);
    return true;
}

void getMethod(string arg, const int clientSocket) { // arg is the filename & docDirectory/Root is terminal input; also check if input arg valid
    if (arg[0] != '/') {
        writeToSocket(clientSocket, "406 Invalid Filename");
        return;
    }        
    if (arg == "/")
        arg = "/homepage.html";

    // Check if docDirectory from terminal input is valid
    filesystem::path path(docDirectory);
    if (!filesystem::is_directory(path)) 
        errorExit("The input document directory is invalid", nullptr);
    
    // Combine file and base directory for a full directory
    string fullDirectory = docDirectory + arg;

    // Opening a file if it exists
    FILE* file = fopen(fullDirectory.c_str(), "rb");
    if (file != nullptr) { // requested file exists
        writeToSocket(clientSocket, "200 OK");
        sendFile(file, clientSocket);
    }
    else
        writeToSocket(clientSocket, "404 File Not Found");
}

bool shutdownMethod(string arg, const int clientSocket) { // arg used to authenticate client
    if (arg != authToken) { // no match
        writeToSocket(clientSocket, "403 Operation Forbidden");
        return false;
    }
    // argument in HTTP request matches -a authToken; shut down
    writeToSocket(clientSocket, "200 Server Shutting Down");
    return true;
}

bool requestLine(string method, string arg, string httpVer, const int clientSocket) {
    // check if "HTTP/"
    if (httpVer.length() != 8 || // ignoring what's after "http/" per the instruction
        (httpVer.length() > 5 && httpVer.substr(0, 5) != "HTTP/")) {
        writeToSocket(clientSocket, "501 Protocol Not Implemented");
        return false;
    }
    if (method == "GET") { // "GET" method requested
        getMethod(arg, clientSocket);
        return true;
    }
    else if (method == "SHUTDOWN") // "SHUTDOWN" method requested
        return shutdownMethod(arg, clientSocket);
    else {
        writeToSocket(clientSocket, "405 Unsupported Method");
        return false;
    }
}

vector<string> httpRequest(const char* buffer, int len, const int clientSocket, bool firstIt) {
    // Parse http request
    std::vector<char> httpResponse;
    bool malRequest = false;
    httpResponse.insert(httpResponse.begin(), buffer, buffer + len); // append data to httpResponse
    if (httpResponse.empty())
        malRequest = true;
    bool rEncountered = false;
    bool firstLine = true;
    vector<string> requestLineElem;
    string word = "";
    for (const char& character : httpResponse) {
        bool isN = (character == '\n');
        bool isR = (character == '\r');
        if (isR)
            rEncountered = true;
        else if ((rEncountered && !isN) || (!rEncountered && isN)) {
            malRequest = true;
            break;
        }
        else if (firstIt && firstLine) {
            if (isN || character == ' ') {    
                bool wordEmpty = word.empty();
                if (!wordEmpty)
                    requestLineElem.push_back(word);
                if (wordEmpty || requestLineElem.size() != 3) {
                    malRequest = true;
                    break;
                }
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
    if (emptyEnd && (emptyLineEnd == true || responseSize == 2))
        requestLineElem.push_back("TRUE");
    else
        malRequest = true;
    if (malRequest)
        requestLineElem.push_back("MAL_REQUEST");
    return requestLineElem;
}

bool clientSocket(const int cs) {
    bool firstIt = true;
    vector<string> request;
    bool notEmpty = false;
    bool malRequest = false;
    while (true) {    
        char buffer[BUFLEN];
        memset(buffer, 0x0, sizeof(buffer)); // Clear the buffer
        size_t receivedLen = recv(cs, buffer, sizeof(buffer), 0);
        if (receivedLen < 0) {
            malRequest = true;
            break;
        }
        // Process the HTTP request in the buffer
        vector<string> returnValues = httpRequest(buffer, receivedLen, cs, firstIt);
        if (returnValues.back() == "MAL_REQUEST") {
            malRequest = true;
            break;
        }
        size_t vecSize = returnValues.size();
        if (vecSize > 2)
            request = returnValues;
        notEmpty = !request.empty();
        if (vecSize == 4 || (vecSize == 1 && notEmpty))
            break;
        else if (vecSize == 3 || (vecSize == 0 && notEmpty))
            continue;
        else {
            malRequest = true;
            break;
        }
        firstIt = false;
    }
    if (notEmpty && !malRequest)
        return requestLine(request[0], request[1], request[2], cs);
    else {
        writeToSocket(cs, "400 Malformed Request");
        return false;
    }
}

/**
 * @brief Establishes the server; taken from socketsd.c example code
 */
void serverConnect() {
    struct sockaddr_in sin; // Stores info about network endpoint for the server
    struct sockaddr addr;
    struct protoent *protocolInfo; // retrieves network protocols; for TCP
    unsigned int addressLen;
    bool closeConnection = false;
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
    
    while (!closeConnection) {
        // accept a connection
        addressLen = sizeof(addr);
        const int sd2 = accept(sd,&addr,&addressLen); // Handles communication with each client
        if (sd2 >= 0) {
            // read, write, and send to the connection (sd2)
            closeConnection = clientSocket(sd2);
            close(sd2);
        } // if invalid, don't send an error message but continue looking for a connection
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