/**
* @file proj3.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for a simple web server for CSDS 325 HW3
* @date 2023-10-05
*/

#include <cstddef>
#include <iostream>
#include <cstring>

/* Defining macros */
#define ERROR 1
#define INT_ERROR -1

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
int N_ARG = -1;
int D_ARG = -1;
int A_ARG = -1;
int port = -1;
char* docDirectory = nullptr;
char* authToken = nullptr;

/* Methods */

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
        else if (i == N_ARG + 1) // FIX: check if these inputs are erroneous later & return error messages
            port = atoi(arg);
        else if (i == D_ARG + 1) {
            docDirectory = new char[strlen(arg)];
            strcpy(docDirectory, arg);
        }
        else if (i == A_ARG + 1) {
            authToken = new char[strlen(arg)];
            strcpy(authToken, arg);
        }
        else
            errorExit("Invalid argument %s", arg);
    }
    if (N_ARG == INT_ERROR || D_ARG == INT_ERROR || A_ARG == INT_ERROR) // Not all options are present
        errorExit("Some or all of the required options are not present", NULL);
    if (port == INT_ERROR || docDirectory == nullptr || authToken == nullptr) // Specifications not all present
        errorExit("Some or all of the required specifications are not present", NULL);
}

void optionN(int port) {
    
}

void optionD() {

}

void optionA() {

}


/**
* @brief Main method for processing the inputs and running appropriate commands
* @param argc Number of arguments
* @param argv Arguments
* @return int Main method return value
*/
int main(int argc, char* argv[]) {
    parseArgs(argc, argv); // When returned, all required inputs present (at least number-wise)

    optionN(port);
}