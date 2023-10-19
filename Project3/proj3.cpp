/**
* @file proj3.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for a simple web server for CSDS 325 HW3
* @date 2023-10-05
*/

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <netdb.h>
#include <system_error>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <dirent.h>

using namespace std;

/* Defining macros */
#define ERROR 1
#define INT_ERROR -1
#define CHAR_SIZE 1
#define R_ONLY 1
#define NRN_ONLY 3
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define START 0
#define EMPTY 0
#define HTTP 5
#define HTTP_LEN 8
#define REQ_ARG 3
#define MIN_RESP 2
#define R_POS 2
#define ARG_FIN 4
#define FIN 1
#define PORT_POS 0
#define DOC_POS 1
#define AUTH_POS 2

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
int N_ARG = INT_ERROR;
int D_ARG = INT_ERROR;
int A_ARG = INT_ERROR;
int serverSocket = INT_ERROR;
int clientSocket = INT_ERROR;
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
    // Close the sockets first
    if (serverSocket >= 0)
        close(serverSocket);
    else if (clientSocket >= 0)
        close(clientSocket);
    // Print the error message & exit
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

/**
 * @brief Sending the input response to the client socket  
 * @param response Response to be sent to the client
 */
void writeToSocket(string response) {
    string retResponse = "HTTP/1.1 " + response + "\r\n\r\n"; // Formatting the response
    char buffer[retResponse.length() + 1]; // String response into char[]
    memset(buffer, 0x0, sizeof(buffer));
    strcpy(buffer, retResponse.c_str());
    if (send(clientSocket, buffer, strlen(buffer), 0) < 0) // Send the response (buffer) to client socket
        errorExit("Error while sending response to the socket", NULL);
}

/**
 * @brief Sending an empty line ('\r\n') to the socket
 */
void sendRN() {
    const char* emptyEnd = "\r\n"; // Manually add \r\n at the end of the sentence or at the very end
    if (send(clientSocket, emptyEnd, strlen(emptyEnd), 0) < 0)
        errorExit("Error while reading the input file", nullptr);
}

/**
 * @brief Sending the requested file to the client socket
 * @param file Requested file
 */
void sendFile(FILE* file) { 
    char buffer[BUFLEN];
    memset(buffer, 0x0, sizeof(buffer)); 
    size_t lenRead;

    while ((lenRead = fread(buffer, CHAR_SIZE, BUFLEN, file)) > 0) { // Check \r\n
        char* line = strchr(buffer, '\n'); // Points at the first '\n' after buffer (as pointer here)    
        if (line == nullptr) { // If super long line; \n not present but still processed
            if (send(clientSocket, buffer, BUFLEN, 0) < 0)
                errorExit("Error while reading the input file", nullptr);
            sendRN();
        }
        while (line != nullptr) {
            size_t lineLen = line - buffer + 1; // length of line to be sent
            char* rPos = strchr(buffer, '\r');
            bool rMissing = false; // \r missing; only \n
            if ((line - rPos) != 1) { // if \r is not before \n
                lineLen -= 1; // Only append up till before \n
                rMissing = true;
            }
            if (send(clientSocket, buffer, lineLen, 0) < 0)
                errorExit("Error while reading the input file", nullptr);
            if (rMissing) { // If \r was missing
                sendRN();
                lineLen += 1; // Add back
            }
            memmove(buffer, line + 1, lenRead - lineLen); // shift up buffer's pointer
            lenRead -= lineLen; // Update total amount left
            line = strchr(buffer, '\n'); // Find the next \n
        }
    }
    sendRN(); // Add an empty line at the end
    fclose(file);
}

/**
 * @brief Called when "GET" method is specified
 * @param filename Argument (file name) of the "GET" method
 */
void getMethod(string filename) {
    if (filename.front() != '/') { // Filename doesn't start with '\'
        writeToSocket("406 Invalid Filename");
        return;
    }        
    if (filename == "/") // If only '\', convert filename to 
        filename = "/homepage.html";

    // Check if docDirectory from terminal input is valid
    DIR *dir = opendir(docDirectory.c_str());
    if (dir == NULL) {
        writeToSocket("404 File Not Found");
        writeToSocket("here?");
        closedir(dir);
        return;
    }

    // Combine file and base directory for a full directory
    string fullDirectory = docDirectory + filename;    
    // Opening a file if it exists
    FILE* file = fopen(fullDirectory.c_str(), "rb");
    if (file != nullptr) { // Requested file exists
        writeToSocket("200 OK");
        sendFile(file);
    }
    else
        writeToSocket("404 File Not Found");
}

/**
 * @brief Called when "SHUTDOWN" method is specified
 * @param arg Argument to the "SHUTDOWN" method for authentication
 * @return true TCP connection is to be closed
 * @return false TCP connection is to kept open
 */
bool shutdownMethod(string arg) {
    if (arg != authToken) { // The argument doesn't match authentication token given in the terminal
        writeToSocket("403 Operation Forbidden");
        return false;
    }
    // Argument in HTTP request matches -a authToken; shut down
    writeToSocket("200 Server Shutting Down");
    return true;
}

/**
 * @brief Processes the request line of the HTTP request sent by the client
 * @param method Method specified in the request
 * @param arg Argument specified in the request
 * @param httpVer HTTP version specified in the request
 * @return true TCP connection is to be closed
 * @return false TCP connection is to kept open
 */
bool requestLine(string method, string arg, string httpVer) {
    // Check if "HTTP/"
    if (httpVer.length() != HTTP_LEN || // Ignoring what's after "http/" per the instruction; just check it's the same length
        (httpVer.length() > HTTP && httpVer.substr(START, HTTP) != "HTTP/")) { // Check if it starts with "HTTP/"
        writeToSocket("501 Protocol Not Implemented");
        return false;
    }
    if (method == "GET") { // "GET" method requested
        getMethod(arg);
        return true; // Connection closed regardless when "GET" called
    }
    else if (method == "SHUTDOWN") // "SHUTDOWN" method requested
        return shutdownMethod(arg);
    else { // Other unsupported method requested
        writeToSocket("405 Unsupported Method");
        return false;
    }
}

/**
 * @brief Processes and parses the HTTP request 
 * @param buffer Contains a part of the request to be processed
 * @param len Length of the content in the buffer 
 * @param firstIt Indicates whether this call is the first iteration of the whole HTTP request processing
 * @return vector<string> Contains the request line and/or indication of whether end of request has been reached or request is malformed 
 */
vector<string> httpRequest(const char* buffer, int len, bool firstIt, bool rEnd) {
    std::vector<char> request; // To store request
    bool malRequest = false; // Flag for whether the request is malformed or not
    request.insert(request.begin(), buffer, buffer + len); // Append buffer to request
    if (request.empty()) // Input buffer is empty; malformed
        malRequest = true;

    bool rEncountered = false; // Indicates whether '\r' is encountered
    bool firstLine = true; // Indicates whether the iteration is on the first line of the request
    vector<string> requestLineElem; // Stores parts of valid request lines
    string word = ""; // To be kept for later processing; words in valid request lines

    if (rEnd && (request.front() != '\n')) {
        requestLineElem.push_back("MAL_REQUEST");
        return requestLineElem;
    }
    // For each character in the request
    for (const char& character : request) { 
        bool isN = (character == '\n'); // Current character is '\n'
        bool isR = (character == '\r'); // Current character is '\r'
        if (!rEnd && ((rEncountered && !isN) || (isN && !rEncountered))) { // If '\r' or '\n' are out-of-order, wrongly-formatted, etc.
            malRequest = true;
            break;
        }
        else if (firstIt && firstLine) { // If first line of the first iteration (chunk) of the whole request to be processed
            if (isN || character == ' ') { // Space or end of sentence reached
                bool wordEmpty = word.empty();
                if (!wordEmpty) // Add the word to vector containing words if word not empty
                    requestLineElem.push_back(word);
                if ((wordEmpty && !rEncountered) || (isN && requestLineElem.size() != REQ_ARG)) { // Empty word & not at end || not 3
                    malRequest = true;
                    break;
                }
                word.clear(); // Clear for next iteration/word
            }
            else if (character != '\r') // Only add to word if not '\r'
                word = word + character;
        }
        rEncountered = isR;
        rEnd = false;
    }
    if (!malRequest) {
        size_t responseSize = request.size(); // Size of request
        bool emptyEnd = ((responseSize >= MIN_RESP) && request.back() == '\n'); // Request ends with \n --> ends with \r\n b/c of check above
        bool emptyLineEnd = (responseSize > MIN_RESP && (request[responseSize - REQ_ARG] == '\n')); // Request ends with an empty line at the end
        if (emptyEnd && (emptyLineEnd == true || responseSize == MIN_RESP)) // Ends correctly && line ends with empty line 
            requestLineElem.push_back("END_REACHED");// check if there's enough empty lines at the end
        else if (request.back() == '\r') // Chunk ends with \r without \n
            requestLineElem.push_back("R_END"); // Require that the next chunk starts with \n or return error
    }
    if (malRequest) // If flagged as a malformed request
        requestLineElem.push_back("MAL_REQUEST");
    return requestLineElem;
}

/**
 * @brief Processes the client socket 
 * @return true TCP connection is to be closed
 * @return false TCP connection is to be kept open
 */
bool clientSocketProcess() {
    bool firstIt = true; // First iteration; calling httpRequest for the first time
    vector<string> request; // Contains the returned values from httpRequest()
    bool malRequest = false; // Flags whether request is malformed or not
    bool rEnd = false;
    bool endReached = false;
    while (!endReached) {    
        char buffer[BUFLEN];
        memset(buffer, 0x0, sizeof(buffer)); // Clear the buffer
        size_t receivedLen = recv(clientSocket, buffer, sizeof(buffer), 0); // FIX; increment
        if (receivedLen < 0) { // Error occurred while reading data from the socket
            malRequest = true;
            break;
        }
        else if (receivedLen == 0)
            break;
        vector<string> returnValues = httpRequest(buffer, receivedLen, firstIt, rEnd);
        if (returnValues.back() == "MAL_REQUEST") { // Flagged as malformed request
            malRequest = true;
            break;
        }
        if (returnValues.back() == "R_END")
            rEnd = true;
        else
            rEnd = false;
        size_t vecSize = returnValues.size(); // How many elements are contained in returnValues vector
        if (vecSize >= REQ_ARG) // Has 3 or 4 elements; request line returned
            request = returnValues;
        if (returnValues.back() == "END_REACHED") // If it indicates that the end of request with an empty line was reached, break loop
            endReached = true;
        firstIt = false; // After first iteration, set to false
    } // FIX; make sure it doesn't end at \r
    if (!malRequest && endReached)
        return requestLine(request[PORT_POS], request[DOC_POS], request[AUTH_POS]);
    else {
        writeToSocket("400 Malformed Request");
        return false;
    }
}

/**
 * @brief Establishes the server; taken from socketsd.c example code
 */
void serverConnect() {
    struct sockaddr_in sin; // Stores info about network endpoint for the server
    struct sockaddr addr;
    struct protoent *protocolInfo; // Retrieves network protocols; for TCP
    unsigned int addressLen;
    bool closeConnection = false;
    int sd, sd2;

    // Determine protocol; only proceed if TCP
    if ((protocolInfo = getprotobyname(PROTOCOL)) == nullptr) // Retrieves TCP information
        errorExit("cannot find protocol information for %s", PROTOCOL);

    // Setup endpoint urlInfo
    memset((char*)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET; // IPv4 address family
    sin.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    sin.sin_port = htons((u_short)stoi(port)); // Set to the port number specified; converted to big endian

    // Allocate a socket; SOCK_STREAM for TCP; would be SOCK_DGRAM for UDP
    sd = socket(PF_INET, SOCK_STREAM, protocolInfo->p_proto);
    serverSocket = sd;

    if (sd < 0)
        errorExit("cannot create socket", nullptr);

    // Bind the socket; local IP address & port 
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errorExit("cannot bind to port %s", port.c_str());

    // Listen for incoming connections; 1 max. pending connections
    if (listen(sd, QLEN) < 0)
        errorExit("cannot listen on port %s\n", port.c_str());

    while (true) {
        // Accept a connection
        addressLen = sizeof(addr);
        if ((sd2 = accept(sd,&addr,&addressLen)) >= 0) { // Handles communication with each client
            clientSocket = sd2;
            // Read, write, and send to the connection (sd2)
            if (clientSocketProcess()) {
                close(sd2);
                break;
            }
        } // If invalid, don't send an error message but continue looking for a connection
        else 
            errorExit("cannot accept client request", nullptr);
    }
    // Close connections and exit    
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
    fprintf(stdout, "Closing the connection... \r\n\r\n");
}