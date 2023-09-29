#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
using namespace std;

// Global variables
int uIndex = -1;
int oIndex = -1;
int dIndex = -1;
int qIndex = -1;
int rIndex = -1;
int urlIndex = -1;
string url;
string urlFile;
string hostname;
string filename;

void parseArgs(int argc, char* argv[]) {
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
        else if (0 == strncasecmp(arg, "wg",2))// FIX
            filename = "";
        else{
            cout << "Invalid argument " << arg << "\n";
            exit(1);
        }
    }
    if (uIndex == -1 || oIndex == -1 || argc < 4) {
        cout << "Valid arguments required; ./proj2 -u URL [-d] [-q] [-r] -o filename\n";
        exit(1); // wrong input; try again with 
    }
}

void optionU(char* arg) {
    url = arg;
    hostname = strtok(arg + 7, "/");
    urlFile = url.substr(size(hostname) + 7, string::npos);
    if (urlFile.empty())
        urlFile = "/";
}

void optionD() {
    cout << "DBG: host: " << hostname << endl;
    cout << "DBG: web_file: " << urlFile << endl;
    cout << "DBG: output_file: " << filename << endl;
}

void optionQ() {

}

void optionR() {

}

void optionO() {

}

// Process arguments from terminal
int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    optionU(argv[urlIndex]);
    if (dIndex != -1)
        optionD();
    optionO();
    if (qIndex != -1)
        optionQ();
    if (rIndex != -1)
        optionR();
}