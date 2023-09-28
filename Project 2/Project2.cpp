/**
 * @file Project2.cpp
 * @author Chaehyeon Kim cxk445
 * @brief Completed for CSDS 325 Project 2
 */

#include <iostream>
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

void optionD() {

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
    string filename;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i]; // argument
        if (arg == "-u")
            uPresent = i;
        else if (arg == "-o")
            oPresent = i;
        else if (arg == "-d")
            dPresent = 1;
        else if (arg == "-q")
            qPresent = 1;
        else if (arg == "-r")
            rPresent = 1;
        else if (arg.substr(0, 2) == "ht")
            url = arg;
        else if (arg.substr(0, 2) == "wg")
            cout << "place holder";
        else
            cout << "Invalid argument " + arg + "\n";
            exit(1);
    }
    if (uPresent == false || oPresent == false || argc < 5) {
        cout << "Valid arguments required; ./proj2 -u URL [-d] [-q] [-r] -o filename\n";
        exit(1); // wrong input; try again with 
    }
    
    optionU();
    optionO();
    if (dPresent)
        optionD();
    if (qPresent)
        optionQ();
    if (rPresent)
        optionR();
}