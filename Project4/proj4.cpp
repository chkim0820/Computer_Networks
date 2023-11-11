/**
* @file proj4.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for CSDS 325 HW4; working with packets
* @date 2023-10-25
*/

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

/* Defining macros */
#define ERROR 1
#define ERROR_INT -1
#define ARG_LENGTH 4
#define MAX_PKT_SIZE 1600
#define VALID_PKT 1
#define NO_PACKET 0

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))

/* Global variables */
string traceFile = "";

/* Structs; taken from next.h */

// meta information, using same layout as trace file
struct meta_info
{
    unsigned int usecs; // microseconds; 10^-6 of a second
    unsigned int secs; // seconds
    unsigned short ignored;
    unsigned short caplen; // Amount of data available
};

// record of information about the current packet
struct pkt_info
{
    unsigned short caplen; // captured length; from meta info
    double now; // from meta info
    unsigned char pkt [MAX_PKT_SIZE];
    struct ether_header *ethh;  // ptr to ethernet header, if present, otherwise NULL
    struct iphdr *iph;          // ptr to IP header, if present, otherwise NULL
    struct tcphdr *tcph;        // ptr to TCP header, if present, otherwise NULL
    struct udphdr *udph;        // ptr to UDP header, if present, otherwise NULL
};

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
 * @brief Parses the arguments given in the terminal 
 * @param argc Number of arguments
 * @param argv Arguments
 * @return string The mode to run the program in
 */
string parseArgs(int argc, char* argv[]) {
    int indexR = ERROR_INT; // Saves the index of the "-r" argument; initialized to -1
    bool optDetermined = false; // Indicates whether mode determined or not; prevent multiple modes
    string mode = "";
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (COMPARE_ARG(arg, "-r") && indexR == ERROR_INT) // arg is "-r" && "-r" not found yet 
            indexR = i;
        else if ((COMPARE_ARG(arg, "-s") || COMPARE_ARG(arg, "-l")
                 || COMPARE_ARG(arg, "-p") || COMPARE_ARG(arg, "-c"))) { // mode not yet called & curr. arg. is one of the modes      
            if (!optDetermined) {
                mode = arg;
                optDetermined = true;
            }
            else 
                errorExit("More than one modes present", nullptr);
        }    
        else if (indexR != ERROR_INT && i == indexR + 1 && !COMPARE_ARG(arg, "-r")) // "-r" was the previous argument; trace file
            traceFile = arg;
        else
            errorExit("Invalid, duplicate, or out-of-order argument %s", arg);
    }
    if (argc != ARG_LENGTH || indexR == ERROR_INT || traceFile.empty() || !optDetermined) // Specifications not all present 
        errorExit("Incorrect arguments; too little or too many", nullptr);
    return mode;
}

/**
 * @brief Method taken from next.c; reads packets and populates a structure
 * @param fd an open file to read packets from
 * @param pinfo allocated memory to put packet info into for one packet
 * @return unsigned short VALID_PKT (1) if a packet was read and pinfo is setup for processing the packet & 
 *                        NO_PACKET (0) if we have hit the end of the file and no packet is available 
 */
unsigned short nextPacket (int fd, struct pkt_info *pinfo) {
    struct meta_info meta; // Stores meta information about packet
    int bytesRead; // bytes read

    // Set memories & initialize fields to 0
    memset(pinfo, 0x0, sizeof(struct pkt_info));
    memset(&meta, 0x0, sizeof(struct meta_info));

    // read the meta information
    bytesRead = read(fd, &meta, sizeof(meta)); // Read fd (file descriptor) into meta
    if (bytesRead == 0) // End of file reached
        return NO_PACKET;
    if (bytesRead < sizeof(meta)) // Smaller number of bytes read than expected
        errorExit("cannot read meta information", nullptr);
    pinfo->caplen = ntohs(meta.caplen); // Convert byte order
    pinfo->now = meta.secs + (double)meta.usecs/1000000; // Timestamp based on meta.secs & meta.usecs

    // Return if the packet is empty or erroneous based on length
    if (pinfo->caplen == 0) // Packet's length equals 0
        return VALID_PKT;
    if (pinfo->caplen > MAX_PKT_SIZE) // Packet is too big
        errorExit("packet too big", nullptr);

    // read the packet contents
    bytesRead = read(fd, pinfo->pkt, pinfo->caplen); // into pinfo's pkt field
    if (bytesRead < 0) // Error occurred
        errorExit("error reading packet", nullptr);
    if (bytesRead < pinfo->caplen) // Length smaller than expected; error
        errorExit("unexpected end of file encountered", nullptr);
    if (bytesRead < sizeof(struct ether_header)) // Not a valid ethernet header
        return VALID_PKT;
    pinfo->ethh = (struct ether_header*)pinfo->pkt; // Store packet as ethernet header struct
    pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type); // Byte-order conversion for ethernet type
    if (pinfo->ethh->ether_type != ETHERTYPE_IP) // Non-IP packets; nothing more to do
        return VALID_PKT;
    if (pinfo->caplen == sizeof(struct ether_header)) // Nothing beyond the ethernet header to process
        return VALID_PKT;

    // Setting IP header & TCP header
    pinfo->iph = (struct iphdr*)(pinfo->pkt + sizeof(struct ether_header)); // Start of IP header
    if (pinfo->iph->protocol == IPPROTO_TCP) { // Check if it is a TCP packet in IP header
        // Setting to the beginning of the TCP header
        pinfo->tcph = (struct tcphdr*)(pinfo->pkt + sizeof(struct ether_header) + sizeof(struct iphdr));
        // Setting up values in tcph struct; FIX (Add more stuff)
        unsigned short scr_port = ntohs(pinfo->tcph->source);
        unsigned short dest_port = ntohs(pinfo->tcph->dest);
    }
    else if (pinfo->iph->protocol == IPPROTO_UDP) { // UDP protocol
        // Setting to the beginning of the UDP header
        pinfo->udph = (struct udphdr*)(pinfo->pkt + sizeof(struct ether_header) + sizeof(struct iphdr));
        // Setting up values in udph struct; FIX (add more stuff)
        unsigned short scr_port = ntohs(pinfo->udph->source);
        unsigned short dest_port = ntohs(pinfo->udph->dest);        
    }
    else 
        errorExit("Non-UDP/TCP packet detected", nullptr);
    return VALID_PKT;
}




// Provides a high-level summary of the trace file; print relevant info
void summaryMode() {

}

void lengthAnalysis() {

}

void packetPrinting() {

}

void packetCounting() {

}

/**
 * @brief The main method that takes in the input arguments and runs the program
 * @param argc Number of arguments
 * @param argv Arguments
 * @return int Return value of the main method
 */
int main(int argc, char* argv[]) {
    string mode = parseArgs(argc, argv); // When returned, all required inputs present (at least number-wise)      
    // Determine which mode from argument parsed above  
    if (mode == "-s")
        summaryMode();
    else if (mode == "-l")
        lengthAnalysis();
    else if (mode == "-p")
        packetPrinting();
    else if (mode == "-c")
        packetCounting();
}