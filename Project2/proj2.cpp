/**
 * @file proj2.cpp
 * @author Chaehyeon Kim cxk445
 * @brief Script for a simple command line-based web client
 * @date 2023-09-25
 */

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>

using namespace std;

/* Define macros */
#define ERROR 1
#define INT_ERROR -1
#define HTTP_ERROR 200
#define REQUIRED_ARGC 5
#define PORT_NUM 80
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define ERROR_POS 9
#define ERROR_LEN 3
#define SKIP_RN 4
#define HTTP_LENGTH 7

/* Comparing arguments case-insensitive */
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(opt)))
/* For building a http request */
#define HTTP_REQUEST(s, maxLen, urlFile, httpVer, hostname) snprintf(s, maxLen, "GET %s HTTP/%s\r\n" \
                                                            "Host: %s\r\n" \
                                                            "User-Agent: CWRU CSDS 325 SimpleClient\r\n" \
                                                            "\r\n", urlFile, httpVer, hostname)

/* Global variables */
int uIndex = INT_ERROR;
int oIndex = INT_ERROR;
int dIndex = INT_ERROR;
int qIndex = INT_ERROR;
int rIndex = INT_ERROR;
int fIndex = INT_ERROR;
int cIndex = INT_ERROR;
char* url = nullptr;
char* urlFile = nullptr;
char* hostname = nullptr;
char* filename = nullptr;

/** Functions */

/**
 * @brief Prints out error messages when called; got from sample.c
 * @param format Given format of the message to be printed
 * @param arg Replaced with the placeholder; NULL if none
 */
void errorExit (const char *format, const char *arg) {
    fprintf (stderr, format, arg);
    fprintf (stderr, "\n");
    exit (ERROR);
}

/**
 * @brief Parse arguments from the main methods
 * @param argc Number of arguments
 * @param argv Array containing arguments
 * @return int integer value of url index
 */
int parseArgs(int argc, char* argv[]) {
    int urlIndex = INT_ERROR;
    // Values to compare each arg to
    for (int i = 1; i < argc; i++) { // go through all arguments
        const char* arg = argv[i];
        if COMPARE_ARG(arg, "-u")
            uIndex = i;
        else if COMPARE_ARG(arg, "-o")
            oIndex = i;
        else if COMPARE_ARG(arg, "-d")
            dIndex = i;
        else if COMPARE_ARG(arg, "-q")
            qIndex = i;
        else if COMPARE_ARG(arg, "-r")
            rIndex = i;
        else if COMPARE_ARG(arg, "-f")
            fIndex = i;
        else if COMPARE_ARG(arg, "-C")
            cIndex = i;
        else if COMPARE_ARG(arg, "http://")
            urlIndex = i;
        else if (i == oIndex + 1) { // FIX
            filename = new char[strlen(arg)];
            strcpy(filename, arg);
        }
        else
            errorExit("Invalid argument %s", arg);
    }
    if (uIndex == INT_ERROR || oIndex == INT_ERROR || argc < REQUIRED_ARGC) // if -u or -o not found or less than 5 args
        errorExit("Valid arguments required:\n./proj2 -u URL [-d] [-q] [-r] -o filename", NULL);
    return urlIndex;
}

/**
 * @brief Helper function for returning correct http version
 * @return string http version to be used depending on -c
 */
string httpVer() {
    string httpVer = "";
    if (cIndex == INT_ERROR)
        httpVer = "1.0";
    else
        httpVer = "1.1";
    return httpVer;
}

/**
 * @brief Sending and receiving http request/response
 * @param buffer buffer from socketConnect()
 * @param sd sd from socketConnect()
 * @return string http response
 */
string httpConnect(char buffer[BUFLEN], int sd) {
    int ret = INT_ERROR;
    // Send an HTTP request
    char http_request[BUFLEN]; // saves http_request
    string httpVer = "1.0";
    if (cIndex != INT_ERROR)
        httpVer = "1.1";
    HTTP_REQUEST(http_request, sizeof(http_request), urlFile, httpVer.c_str(), hostname);
    if (send(sd, http_request, strlen(http_request), 0) < 0)
        errorExit("cannot send", NULL);

    // Loop for getting responses
    std::vector<char> httpResponse;
    while (true) {
        memset(buffer, 0x0, BUFLEN);
        ret = read(sd, buffer, BUFLEN - 1);
        if (ret < 0)
            errorExit("reading error", NULL);
        else if (ret == 0) // end of loop; break
            break;
        else
            httpResponse.insert(httpResponse.end(), buffer, buffer + ret); // append data to httpResponse
    }
    string retResponse(httpResponse.begin(), httpResponse.end());
    return retResponse;
}

/**
 * @brief Creating socket and sending http request & receiving http response; modified from sockets.c
 * @return string http response
 */
string socketConnect() {
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *procinfo;
    char buffer[BUFLEN];
    int sd;

    //lookup the hostname
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errorExit("cannot find name: %s", hostname);

    // set endpoint information
    memset((char*)&sin, 0x0, sizeof(sin)); // for memory
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT_NUM); // for http
    memcpy ((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);
    if ((procinfo = getprotobyname(PROTOCOL)) == NULL)
        errorExit ("cannot find protocol information for %s", PROTOCOL);

    // allocate a socket
    sd = socket(PF_INET, SOCK_STREAM, procinfo->p_proto);
    if (sd < 0)
        errorExit("cannot create socket",NULL);

    // connect the socket
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errorExit("cannot connect", NULL);

    // http request & response
    string response = httpConnect(buffer, sd);

    // close & return http response
    close(sd);
    return response;
}

/**
 * @brief Executes option U when called; assigns appropriate variables 
 * @param arg url followed by -u in the input
 */
void optionU(char* arg) {
    // Safety feature; deallocate if previously allocated
    if (url != nullptr)
        delete[] url;
    if (hostname != nullptr)
        delete[] hostname;
    if (urlFile != nullptr)
        delete[] urlFile;

    // Allocating memory
    const int size = strlen(arg) + 1; 
    url = new char[size];
    hostname = new char[size];
    urlFile = new char[size];

    // Copying whole arg into url
    strcpy(url, arg);

    // Assigning hostname from arg
    char* tok = strtok(arg + HTTP_LENGTH, "/"); // Skip "http://"
    if (tok != nullptr)
        strcpy(hostname, tok);
    else // nothing after 'http://'
        errorExit("No valid hostname could be found", NULL); 

    // Assigning urlFile if exists (optional)
    strcpy(urlFile, url + strlen(arg));
    if (urlFile[0] == '\0' || urlFile == nullptr)
        strcpy(urlFile, "/");
}

/**
 * @brief Executes option D when called; prints out hostname, web_file, and output_file
 */
void optionD() {
    fprintf(stdout, "DBG: host: %s\n", hostname);
    fprintf(stdout, "DBG: web_file: %s\n", urlFile);
    fprintf(stdout, "DBG: output_file: %s\n", filename);
}

/**
 * @brief Executes option Q when called; printing http request
 */
void optionQ() {
    fprintf(stdout, "OUT: GET %s HTTP/1.0\r\n", urlFile);
    fprintf(stdout, "OUT: Host: %s\r\n", hostname);
    fprintf(stdout, "OUT: User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n");
}

/**
 * @brief Executes option R when called; printing http response
 * @param response received http response
 */
void optionR(const char* response) {
    // Creating a temporary variable containing everything after "\r\n\r\n"; strtok can't differentiate duplicated \r\n
    char* tempResp = new char[strlen(response) + 1];
    strcpy(tempResp, response); // copy over original response
    char* stopping = strstr(tempResp, "\r\n\r\n"); // everything after empty line; stopping point
    for (int i = 0; *stopping && i < SKIP_RN; i++) // go to the first character after "\r\n\r\n"
        stopping++;
    tempResp[strlen(tempResp) - strlen(stopping)] = '\0'; // char first one after stopping

    // Output line by line
    char* line = strtok(tempResp, "\r\n");
    while (line != nullptr) { // until empty line with only "\r\n"
        fprintf(stdout, "INC: %s\r\n", line);
        line = strtok(nullptr, "\r\n");
    }
}

/**
 * @brief Executes option O when called; saves contents to filename
 * @param response received http response
 */
int optionO(string response) {
    int start = response.find("\r\n\r\n") + SKIP_RN; // find the empty line & skip over that line
    string downloaded = response.substr(start, string::npos); // after empty line to end
    int errCode = stoi(response.substr(ERROR_POS, ERROR_LEN)); // fetch error code
    if (errCode != HTTP_ERROR && fIndex < 0)
        errorExit("ERROR: non-200 response code", filename);

    // Save contents to designated file name
    ofstream toFile(filename, ios::binary);
    toFile.write(downloaded.c_str(), downloaded.size());
    toFile.close();
    return errCode;
}

/**
 * @brief Executed when option f called; redirects to the url after "Location: "
 * @param response http response
 * @return string url to be redirected to
 */
string optionF(string response) {
    int start = response.find("Location: ") + strlen("Location: "); // find the empty line & skip over that line
    int end = response.find("\r\n", start);
    string redirect = response.substr(start, end - start);
    return redirect;
}

/**
 * @brief For when -C option is called; wasn't implemented 
 */
void optionC() {

}

/**
 * @brief The main method 
 * @param argc number of arguments
 * @param argv array containing arguments
 * @return int returns int
 */
int main(int argc, char* argv[]) {
    int urlIndex = parseArgs(argc, argv);
    char* url = argv[urlIndex];

    while (true) {
        optionU(url);
        if (dIndex != INT_ERROR)
            optionD();

        string httpResponse = socketConnect();
        if (qIndex != INT_ERROR)
            optionQ();
        if (rIndex != INT_ERROR)
            optionR(httpResponse.c_str());
        int errCode = optionO(httpResponse);

        // Extra credits
        if (errCode == 200)
            break;
        else {
            string tempURL = optionF(httpResponse);
            url = new char[tempURL.length()];
            strcpy(url, tempURL.c_str());
        }
    }
}