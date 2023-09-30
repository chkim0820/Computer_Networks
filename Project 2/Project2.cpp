#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>

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

#define ERROR 1
#define REQUIRED_ARGC 3
#define HOST_POS 1
#define PORT_POS 2
#define PROTOCOL "tcp"
#define BUFLEN 1024

int parseArgs(int argc, char* argv[]) {
    int urlIndex = -1;
    for (int i = 1; i < argc; i++) { // go through all arguments
        char* arg = argv[i];
        if (0 == strncasecmp(arg, "-u", 2)) // account for diff. cases
            uIndex = i;
        else if (0 == strncasecmp(arg, "-o", 2))
            oIndex = i;
        else if (0 == strncasecmp(arg, "-d", 2))
            dIndex = i;
        else if (0 == strncasecmp(arg, "-q", 2))
            qIndex = i;
        else if (0 == strncasecmp(arg, "-r", 2))
            rIndex = i;
        else if (0 == strncasecmp(arg, "http://",7))
            urlIndex = i;
        else if (i == oIndex + 1) { // FIX
            filename = new char[strlen(arg)];
            strcpy(filename, arg);
        }
        else{
            fprintf(stderr, "Invalid argument %s\n", arg);
            exit(ERROR);
        }
    }
    if (uIndex == -1 || oIndex == -1 || argc < 5) { // FIX: check for url and filename, too?
        fprintf(stderr, "Valid arguments required; ./proj2 -u URL [-d] [-q] [-r] -o filename\n");
        exit(ERROR); // wrong input; try again with 
    }
    return urlIndex;
}

int errorExit (char *format, char *arg)
{
    fprintf (stderr,format,arg);
    fprintf (stderr,"\n");
    exit (ERROR);
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
    else { // Nothing after "http://"
        fprintf(stderr, "No valid hostname could be found");    
        exit(ERROR);
    }

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
    // int sd = socket(PF_INET, SOCK_STREAM, );


    // string httpVer; 
    // cout << "GET " << urlFile << " HTTP/" << httpVer << "\r\n";
    // cout << "Host: " << hostname << "\r\n";
    // cout << "User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n";
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

    // to implement:
    if (qIndex != -1)
        optionQ();
    optionO();
    if (rIndex != -1)
        optionR();
}