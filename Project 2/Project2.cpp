/**
 * @file Project2.cpp
 * @author Chaehyeon Kim cxk445
 * @brief Completed for CSDS 325 Project 2
 */

#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
using namespace std;

class Project2 {
    private:
        void optionU();
        void optionD();
        void optionQ();
        void optionR();
        void optionO();
};

void optionU() {

}

void optionD(string urlFile, string hostname, string filename) {
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
    bool uPresent = false;
    bool oPresent = false;
    bool dPresent = false;
    bool qPresent = false;
    bool rPresent = false;

    string url;
    string urlFile;
    string hostname;
    string filename;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (0 == strncasecmp(argv[i], "-u", 2)) // account for diff. cases
            uPresent = true;
        else if (0 == strncasecmp(argv[i], "-o", 2))
            oPresent = true;
        else if (0 == strncasecmp(argv[i], "-d", 2))
            dPresent = true;
        else if (0 == strncasecmp(argv[i], "-q", 2))
            qPresent = true;
        else if (0 == strncasecmp(argv[i], "-r", 2))
            rPresent = true;
        else if (0 == strncasecmp(argv[i], "http://",7)){
            url = arg;
            // char* tok = strtok(argv[i] + 7, "/");
            // hostname = tok;
            hostname = strtok(argv[i] + 7, "/");
            urlFile = arg.substr(size(hostname) + 7, string::npos);
            if (urlFile.empty())
                urlFile = "/";

            // cout << hostname << " is hostname" << endl;
            // cout << url << " is url" << endl;
            // cout << urlFile << " is the urlFile" <<endl;
        }
        else if (0 == strncasecmp(argv[i], "wg",2))// FIX
            filename = "";
        else{
            cout << "Invalid argument " << argv[i] << "\n";
            exit(1);
        }
    }
    if (uPresent == false || oPresent == false || argc < 4) {
        cout << "Valid arguments required; ./proj2 -u URL [-d] [-q] [-r] -o filename\n";
        exit(1); // wrong input; try again with 
    }

    if (dPresent)
        optionD(urlFile, hostname, filename);

    optionU();
    optionO();
    
    if (qPresent)
        optionQ();
    if (rPresent)
        optionR();
}