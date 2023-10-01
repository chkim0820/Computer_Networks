// literals
// buffer size?
// general organization
// extra libraries?
// memories?
// o option outputs? error
// ./proj2 -r -u http://case.edu/ -o case.html?


/**
 * @file Project2.cpp
 * @author Chaehyeon Kim cxk445
 */

#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

/* Define macros */
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(opt)))
#define ERROR 1
#define INT_ERROR -1
#define HTTP_ERROR 200
#define REQUIRED_ARGC 5
#define PORT_NUM 80
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define HTTP_REQUEST(s, maxLen, urlFile, hostname) \
        snprintf(s, maxLen, "GET %s HTTP/1.0\r\n" \
                "Host: %s\r\n" \
                "User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n" \
                "\r\n", urlFile, hostname)

/* Global variables */
int uIndex = INT_ERROR;
int oIndex = INT_ERROR;
int dIndex = INT_ERROR;
int qIndex = INT_ERROR;
int rIndex = INT_ERROR;
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
    const char* u = "-u";
    const char* o = "-o";
    const char* d = "-d";
    const char* q = "-q";
    const char* r = "-r";
    const char* http = "http://";
    for (int i = 1; i < argc; i++) { // go through all arguments
        const char* arg = argv[i];
        if COMPARE_ARG(arg, u)
            uIndex = i;
        else if COMPARE_ARG(arg, o)
            oIndex = i;
        else if COMPARE_ARG(arg, d)
            dIndex = i;
        else if COMPARE_ARG(arg, q)
            qIndex = i;
        else if COMPARE_ARG(arg, r)
            rIndex = i;
        else if COMPARE_ARG(arg, http)
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
 * @brief Creating socket and sending http request & receiving http response; modified from sockets.c
 * @return string http response
 */
string httpConnect() {
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *procinfo;
    char buffer [BUFLEN];
    int sd, ret;

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

    // Send an HTTP request
    char http_request[BUFLEN]; // saves http_request
    HTTP_REQUEST(http_request, sizeof(http_request), urlFile, hostname);
    if (send(sd, http_request, strlen(http_request), 0) < 0)
        errorExit("cannot send", NULL);

    // snarf whatever server provides and print it
    memset(buffer,0x0,BUFLEN);
    ret = read(sd,buffer,BUFLEN - 1);
    if (ret < 0)
        errorExit("reading error",NULL);

    // close & return http response
    close(sd);
    string httpResponse = buffer;
    return httpResponse;
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
    url = new char[strlen(arg) + 1];
    hostname = new char[strlen(arg)];
    urlFile = new char[strlen(arg)];

    // Copying whole arg into url
    strcpy(url, arg);

    // Assigning hostname from arg
    char* tok = strtok(arg + 7, "/"); // Skip "http://"
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
    // Printing outputs //FIX so that OUT: is directly added to the actual input
    fprintf(stdout, "OUT: GET %s HTTP/1.0\r\n", urlFile); //FIX
    fprintf(stdout, "OUT: Host: %s\r\n", hostname);
    fprintf(stdout, "OUT: User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n");
}

/**
 * @brief Executes option R when called; printing http response
 * @param response received http response
 */
void optionR(const char* response) {
    // Creating a temporary variable containing everything after "\r\n\r\n"; strtok can't differentiate duplicated \r\n
    char* tempResp = new char[strlen(response) + 1]; // allocate memory
    strcpy(tempResp, response); // copy over original response
    char* stopping = strstr(tempResp, "\r\n\r\n"); // everything after empty line; stopping point
    for (int i = 0; *stopping && i < 4; i++) // go to the first character after "\r\n\r\n"
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
void optionO(string response) {
    int start = response.find("\r\n") + 4; // find the empty line & skip over that line
    string downloaded = response.substr(start, string::npos); // after empty line to end
    int errCode = stoi(response.substr(9, 3)); // fetch error code
    if (errCode != HTTP_ERROR)
        errorExit("ERROR: non-200 response code", filename);

    // Save contents to designated file name
    ofstream toFile(filename);
    toFile << downloaded;
    toFile.close();
}

/**
 * @brief The main method 
 * @param argc number of arguments
 * @param argv array containing arguments
 * @return int returns int
 */
int main(int argc, char* argv[]) {
    int urlIndex = parseArgs(argc, argv); // maybe urlIndex could be returned instead?

    optionU(argv[urlIndex]);
    if (dIndex != INT_ERROR)
        optionD();

    string httpResponse = httpConnect();
    if (qIndex != INT_ERROR)
        optionQ();
    if (rIndex != INT_ERROR)
        optionR(httpResponse.c_str());
    optionO(httpResponse);
}