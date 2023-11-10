/**
* @file proj4.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for CSDS 325 HW4; working with packets
* @date 2023-10-25
*/

#include <iostream>
#include <cstring>

using namespace std;

/* Defining macros */
#define ERROR 1
#define ERROR_INT -1
#define ARG_LENGTH 4

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
string traceFile = "";
string option = "";

/* Methods */

/**
 * @brief Prints out error messages when called; taken from sample.c
 * @param format Given format of the message to be printed
 * @param arg Replaced with the placeholder; NULL if none
 */
void errorExit (const char *format, const char *arg) {
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
    int optionR = ERROR_INT; // Saves the index of the "-r" argument; initialized to -1
    bool optDetermined = false; // Indicates whether option determined or not; prevent multiple options
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (COMPARE_ARG(arg, "-r") && optionR == ERROR_INT) // arg is "-r" && "-r" not found yet 
            optionR = i;
        else if ((COMPARE_ARG(arg, "-s") || COMPARE_ARG(arg, "-l")
                 || COMPARE_ARG(arg, "-p") || COMPARE_ARG(arg, "-c"))) { // option not yet called & curr. arg. is one of the options      
            if (!optDetermined) {
                option = arg;
                optDetermined = true;
            }
            else 
                errorExit("More than one options present", nullptr);
        }    
        else if (optionR != ERROR_INT && i == optionR + 1 && !COMPARE_ARG(arg, "-r")) // "-r" was the previous argument; trace file
            traceFile = arg;
        else
            errorExit("Invalid, duplicate, or out-of-order argument %s", arg);
    }
    if (argc != ARG_LENGTH || optionR == ERROR_INT || traceFile.empty() || !optDetermined) // Specifications not all present 
        errorExit("Incorrect arguments; too little or too many", nullptr);
}


/**
 * @brief The main method that takes in the input arguments and runs the program
 * @param argc Number of arguments
 * @param argv Arguments
 * @return int Return value of the main method
 */
int main(int argc, char* argv[]) {
    parseArgs(argc, argv); // When returned, all required inputs present (at least number-wise)
    cout << "correct argument!" <<endl;
}