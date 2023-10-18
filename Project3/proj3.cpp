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

/**
 * @brief Sending the input response to the client socket  
 * @param clientSocket Socket to be returned to the client
 * @param response Response to be sent to the client
 */
void writeToSocket(const int clientSocket, string response) {
    string retResponse = "HTTP/1.1 " + response + "\r\n\r\n"; // Formatting the response
    char buffer[retResponse.length() + 1]; // String response into char[]
    memset(buffer, 0x0, sizeof(buffer));
    strcpy(buffer, retResponse.c_str());
    if (send(clientSocket, buffer, strlen(buffer), 0) < 0) // Send the response (buffer) to client socket
        errorExit("Error while sending response to the socket", NULL);
}

/**
 * @brief Sending the requested file to the client socket
 * @param file Requested file
 * @param clientSocket Socket to be returned to the client
 */
void sendFile(FILE* file, const int clientSocket) { 
    char buffer[BUFLEN];
    memset(buffer, 0x0, sizeof(buffer)); 
    size_t lenRead;

    while ((lenRead = fread(buffer, CHAR_SIZE, BUFLEN, file)) > 0) { // Check \r\n
        char* line = strchr(buffer, '\n'); // Points at the first '\n' after buffer (as pointer here)    
        if (line == nullptr && lenRead == sizeof(buffer)) { // If super long line; \n not present but still processed
            if (send(clientSocket, buffer, BUFLEN, 0) < 0)
                errorExit("Error while reading the input file", nullptr);
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
                const char* emptyEnd = "\r\n"; // Manually add \r\n at the end of the sentence
                if (send(clientSocket, emptyEnd, strlen(emptyEnd), 0) < 0)
                    errorExit("Error while reading the input file", nullptr);
                lineLen += 1; // Add back
            }
            memmove(buffer, line + 1, lenRead - lineLen); // shift up buffer's pointer
            lenRead -= lineLen; // Update total amount left
            line = strchr(buffer, '\n'); // Find the next \n
        }
    }
    const char* emptyLine = "\r\n";
    if (send(clientSocket, emptyLine, strlen(emptyLine), 0) < 0)
        errorExit("Error while adding an empty line to the client socket", nullptr);
    fclose(file);
}

/**
 * @brief Called when "GET" method is specified
 * @param filename Argument (file name) of the "GET" method
 * @param clientSocket Socket to be returned to the client
 */
void getMethod(string filename, const int clientSocket) {
    if (filename.front() != '/') { // Filename doesn't start with '\'
        writeToSocket(clientSocket, "406 Invalid Filename");
        return;
    }        
    if (filename == "/") // If only '\', convert filename to 
        filename = "/homepage.html";

    // Check if docDirectory from terminal input is valid
    filesystem::path path(docDirectory);
    if (!filesystem::is_directory(path)) 
        errorExit("The input document directory is invalid", nullptr);
    
    // Combine file and base directory for a full directory
    string fullDirectory = docDirectory + filename;

    // Opening a file if it exists
    FILE* file = fopen(fullDirectory.c_str(), "rb");
    if (file != nullptr) { // Requested file exists
        writeToSocket(clientSocket, "200 OK");
        sendFile(file, clientSocket);
    }
    else
        writeToSocket(clientSocket, "404 File Not Found");
}

/**
 * @brief Called when "SHUTDOWN" method is specified
 * @param arg Argument to the "SHUTDOWN" method for authentication
 * @param clientSocket Socket to be returned to the client
 * @return true TCP connection is to be closed
 * @return false TCP connection is to kept open
 */
bool shutdownMethod(string arg, const int clientSocket) {
    if (arg != authToken) { // The argument doesn't match authentication token given in the terminal
        writeToSocket(clientSocket, "403 Operation Forbidden");
        return false;
    }
    // Argument in HTTP request matches -a authToken; shut down
    writeToSocket(clientSocket, "200 Server Shutting Down");
    return true;
}

/**
 * @brief Processes the request line of the HTTP request sent by the client
 * @param method Method specified in the request
 * @param arg Argument specified in the request
 * @param httpVer HTTP version specified in the request
 * @param clientSocket Socket to be returned to the client 
 * @return true TCP connection is to be closed
 * @return false TCP connection is to kept open
 */
bool requestLine(string method, string arg, string httpVer, const int clientSocket) {
    // Check if "HTTP/"
    if (httpVer.length() != HTTP_LEN || // Ignoring what's after "http/" per the instruction; just check it's the same length
        (httpVer.length() > HTTP && httpVer.substr(START, HTTP) != "HTTP/")) { // Check if it starts with "HTTP/"
        writeToSocket(clientSocket, "501 Protocol Not Implemented");
        return false;
    }
    if (method == "GET") { // "GET" method requested
        getMethod(arg, clientSocket);
        return true; // Connection closed regardless when "GET" called
    }
    else if (method == "SHUTDOWN") // "SHUTDOWN" method requested
        return shutdownMethod(arg, clientSocket);
    else { // Other unsupported method requested
        writeToSocket(clientSocket, "405 Unsupported Method");
        return false;
    }
}

/**
 * @brief Processes and parses the HTTP request 
 * @param buffer Contains a part of the request to be processed
 * @param len Length of the content in the buffer 
 * @param clientSocket Socket to be returned to the client 
 * @param firstIt Indicates whether this call is the first iteration of the whole HTTP request processing
 * @return vector<string> Contains the request line and/or indication of whether end of request has been reached or request is malformed 
 */
vector<string> httpRequest(const char* buffer, int len, const int clientSocket, bool firstIt) {
    std::vector<char> request; // To store request
    bool malRequest = false; // Flag for whether the request is malformed or not
    request.insert(request.begin(), buffer, buffer + len); // Append buffer to request
    if (request.empty()) // Input buffer is empty; malformed
        malRequest = true;

    bool rEncountered = false; // Indicates whether '\r' is encountered
    bool firstLine = true; // Indicates whether the iteration is on the first line of the request
    vector<string> requestLineElem; // Stores parts of valid request lines
    string word = ""; // To be kept for later processing; words in valid request lines

    // For each character in the request
    for (const char& character : request) { 
        bool isN = (character == '\n'); // Current character is '\n'
        bool isR = (character == '\r'); // Current character is '\r'
        if (isR)
            rEncountered = true;
        else if ((rEncountered && !isN) || (!rEncountered && isN)) { // If '\r' or '\n' are out-of-order, wrongly-formatted, etc.
            malRequest = true;
            break;
        }
        else if (firstIt && firstLine) { // If first line of the first iteration (chunk) of the whole request to be processed
            if (isN || character == ' ') { // Space or end of sentence reached
                bool wordEmpty = word.empty();
                if (!wordEmpty) // Add the word to vector containing words if word not empty
                    requestLineElem.push_back(word);
                if (isN && (wordEmpty || requestLineElem.size() != REQ_ARG)) { // At the end of line && not 3 words added; malformed request
                    malRequest = true;
                    break;
                }
                word.clear(); // Clear for next iteration/word
            }
            else
                word = word + character;
        }
        if (!isR) // reset
            rEncountered = false;
    }
    if (!malRequest) { // FIX: cut off at \r and just \n
        size_t responseSize = request.size(); // Size of request
        bool emptyEnd = ((responseSize >= MIN_RESP) && request.back() == '\n'); // Request ends with \n --> ends with \r\n b/c of check above
        bool emptyLineEnd = (responseSize > MIN_RESP && (request[responseSize - REQ_ARG] == '\n')); // Request ends with an empty line at the end
        if (emptyEnd && (emptyLineEnd == true || responseSize == MIN_RESP)) // Ends correctly && line ends with empty line 
            requestLineElem.push_back("TRUE");
    }
    if (malRequest) // If flagged as a malformed request
        requestLineElem.push_back("MAL_REQUEST");
    return requestLineElem;
} // FIX: could also not end with \r\n b/c of the body

/**
 * @brief Processes the client socket 
 * @param cs Socket to be returned to the client 
 * @return true TCP connection is to be closed
 * @return false TCP connection is to be kept open
 */
bool clientSocket(const int cs) {
    bool firstIt = true; // First iteration; calling httpRequest for the first time
    vector<string> request; // Contains the returned values from httpRequest()
    bool notEmpty = false; // Flags whether request (above) is empty or not
    bool malRequest = false; // Flags whether request is malformed or not
    while (true) {    
        char buffer[BUFLEN];
        memset(buffer, 0x0, sizeof(buffer)); // Clear the buffer
        size_t receivedLen = recv(cs, buffer, sizeof(buffer), 0);
        if (receivedLen < 0) { // Error occurred while reading data from the socket
            malRequest = true;
            break;
        }
        // Process the HTTP request in the buffer
        vector<string> returnValues = httpRequest(buffer, receivedLen, cs, firstIt);
        if (returnValues.back() == "MAL_REQUEST") { // Flagged as malformed request
            malRequest = true;
            break;
        }
        size_t vecSize = returnValues.size(); // How many elements are contained in returnValues vector
        if (vecSize >= REQ_ARG) // Has 3 or 4 elements; request line returned
            request = returnValues;
        notEmpty = !request.empty(); // Vector is not empty
        if (vecSize == ARG_FIN || (vecSize == FIN && notEmpty)) // If it indicates that the end of request with an empty line was reached, break loop
            break;
        else if (vecSize == REQ_ARG || (vecSize == EMPTY && notEmpty)) // If the end of request has not been reached, keep looping
            continue;
        else { // Else, flag as malformed request
            malRequest = true;
            break;
        }
        firstIt = false; // After first iteration, set to false
    }
    if (notEmpty && !malRequest)
        return requestLine(request[PORT_POS], request[DOC_POS], request[AUTH_POS], cs);
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
    struct protoent *protocolInfo; // Retrieves network protocols; for TCP
    unsigned int addressLen;
    bool closeConnection = false;

    // Determine protocol; only proceed if TCP
    if ((protocolInfo = getprotobyname(PROTOCOL)) == nullptr) // Retrieves TCP information
        errorExit("cannot find protocol information for %s", PROTOCOL);

    // Setup endpoint urlInfo
    memset((char*)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET; // IPv4 address family
    sin.sin_addr.s_addr = INADDR_ANY; // Bind to all available network interfaces
    sin.sin_port = htons((u_short)stoi(port)); // Set to the port number specified; converted to big endian

    // Allocate a socket; SOCK_STREAM for TCP; would be SOCK_DGRAM for UDP
    int sd = socket(PF_INET, SOCK_STREAM, protocolInfo->p_proto);
    if (sd < 0)
        errorExit("cannot create socket", nullptr);

    // Bind the socket; local IP address & port 
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errorExit("cannot bind to port %s", port.c_str());

    // Listen for incoming connections; 1 max. pending connections
    if (listen(sd, QLEN) < 0)
        errorExit("cannot listen on port %s\n", port.c_str());
    
    while (!closeConnection) {
        // Accept a connection
        addressLen = sizeof(addr);
        const int sd2 = accept(sd,&addr,&addressLen); // Handles communication with each client
        if (sd2 >= 0) {
            // Read, write, and send to the connection (sd2)
            closeConnection = clientSocket(sd2);
            close(sd2);
        } // If invalid, don't send an error message but continue looking for a connection
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
}