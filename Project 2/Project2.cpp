#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>


using namespace std;

// Global variables
int uIndex = -1;
int oIndex = -1;
int dIndex = -1;
int qIndex = -1;
int rIndex = -1;
char* url = nullptr;
char* urlFile = nullptr;
char* hostname = nullptr;
char* filename = nullptr;

#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(opt)))

#define ERROR 1
#define REQUIRED_ARGC 3
#define PORT_NUM 80
#define PROTOCOL "tcp"
#define BUFLEN 1024

int errorExit (const char *format, const char *arg) {
    fprintf (stderr, format, arg);
    fprintf (stderr, "\n");
    exit (ERROR);
}

int parseArgs(int argc, char* argv[]) {
    int urlIndex = -1;
    for (int i = 1; i < argc; i++) { // go through all arguments
        char* arg = argv[i];
        if COMPARE_ARG(arg, "-u") // FIX; use define?
            uIndex = i;
        else if COMPARE_ARG(arg, "-o")
            oIndex = i;
        else if COMPARE_ARG(arg, "-d")
            dIndex = i;
        else if COMPARE_ARG(arg, "-q")
            qIndex = i;
        else if COMPARE_ARG(arg, "-r")
            rIndex = i;
        else if COMPARE_ARG(arg, "http://")
            urlIndex = i;
        else if (i == oIndex + 1) { // FIX
            filename = new char[strlen(arg)];
            strcpy(filename, arg);
        }
        else
            errorExit("Invalid argument %s", arg);
    }
    if (uIndex == -1 || oIndex == -1 || argc < 5)
        errorExit("Valid arguments required:\n./proj2 -u URL [-d] [-q] [-r] -o filename", NULL);
    return urlIndex;
}

void httpConnect() {
    struct sockaddr_in sin;
    struct hostent *hinfo;
    struct protoent *procinfo;
    char buffer [BUFLEN];
    int sd, ret;

    /* lookup the hostname */
    hinfo = gethostbyname(hostname);
    if (hinfo == NULL)
        errorExit("cannot find name: %s", hostname);

    /* set endpoint information */
    memset((char*)&sin, 0x0, sizeof(sin)); // for memory
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT_NUM); // for http
    memcpy ((char *)&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    if ((procinfo = getprotobyname(PROTOCOL)) == NULL)
        errorExit ("cannot find protocol information for %s", PROTOCOL);

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, procinfo->p_proto);
    if (sd < 0)
        errorExit("cannot create socket",NULL);

    /* connect the socket */
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errorExit("cannot connect", NULL);

    /* Snd an HTTP request */
    char http_request[BUFLEN];
    snprintf(http_request, sizeof(http_request), 
        "GET %s HTTP/1.0\r\n"
                "Host: %s\r\n"
                "User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n"
                "\r\n",
             urlFile, hostname);
    if (send(sd, http_request, strlen(http_request), 0) < 0)
        errorExit("cannot send", NULL);

    /* snarf whatever server provides and print it */
    memset(buffer,0x0,BUFLEN);
    ret = read(sd,buffer,BUFLEN - 1);
    if (ret < 0)
        errorExit("reading error",NULL);
   
    /* close & exit */
    close(sd);
}

void optionU(char* arg) {
    // Safety feature; deallocate if previously allocated
    if (url != nullptr)
        delete[] url;
    if (hostname != nullptr)
        delete[] hostname;
    if (urlFile != nullptr)
        delete[] urlFile;

    // Copying whole arg into url
    url = new char[strlen(arg) + 1]; // Saves the whole url
    strcpy(url, arg);

    // Assigning hostname from arg
    char* tok = strtok(arg + 7, "/"); // Skip "http://"
    hostname = new char[strlen(tok) + 1];
    if (tok != nullptr)
        strcpy(hostname, tok);
    else // nothing after 'http://'
        errorExit("No valid hostname could be found", NULL); 

    // Assigning urlFile if exists
    urlFile = new char[strlen(arg)];
    strcpy(urlFile, url + strlen(arg));
    if (urlFile[0] == '\0' || urlFile == nullptr)
        strcpy(urlFile, "/");
}

void optionD() {
    fprintf(stdout, "DBG: host: %s\n", hostname);
    fprintf(stdout, "DBG: web_file: %s\n", urlFile);
    fprintf(stdout, "DBG: output_file: %s\n", filename);
}

void optionQ() {
    // Printing outputs
    fprintf(stdout, "OUT: GET %s HTTP/1.0\r\n", urlFile); //FIX
    fprintf(stdout, "OUT: Host: %s\r\n", hostname);
    fprintf(stdout, "OUT: User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n");
}



void optionR() {

}

void optionO() {

}

// Process arguments from terminal
int main(int argc, char* argv[]) {
    int urlIndex = parseArgs(argc, argv); // maybe urlIndex could be returned instead?

    optionU(argv[urlIndex]);
    if (dIndex != -1)
        optionD();

    if (qIndex != -1){
        httpConnect();
        optionQ();
    }
    optionO();
    if (rIndex != -1)
        optionR();
}