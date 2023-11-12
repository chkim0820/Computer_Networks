/**
* @file proj4.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for CSDS 325 HW4; working with packets
* @date 2023-10-25
*/

#include <iterator>
#include <stdio.h>
#include <string>
#include <tuple>
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
#define PADDING 6
#define BYTE 4
#define META_INFO_BYTES 12
#define PKT_BF_IP 34
#define FOUR_BYTES 32

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))
// Converting a double number to const char to print
#define INT_PRINT(num) (to_string(num).c_str())
// Converting an int number to const char to print
#define DOUBLE_PRINT(num, dec) (truncDecimal(num, dec).c_str())

/* Structs; taken from next.h */

// meta information, using same layout as trace file
struct meta_info
{
    unsigned int usecs; // microseconds; 10^-6 of a second
    unsigned int secs; // seconds
    unsigned short ignored;
    unsigned short caplen; // Amount of data available in the packet portion (doesn't include meta-info)
};

// record of information about the current packet
struct pkt_info
{
    unsigned short caplen; // captured length; from meta info
    double now; // time combining usecs and secs from meta info
    unsigned char pkt[MAX_PKT_SIZE]; // Stores the whole packet after meta-info
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
 * @return tuple<string, string> mode and tracefile extracted
 */
tuple<string, string> parseArgs(int argc, char* argv[]) {
    int indexR = ERROR_INT; // Saves the index of the "-r" argument; initialized to -1
    bool optDetermined = false; // Indicates whether mode determined or not; prevent multiple modes
    string mode = "";
    string traceFile = "";
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
    return make_tuple(mode, traceFile);
}

/**
 * @brief Truncating the input number to appropriate decimal place 
 * @param num Input number to be truncated
 * @param decimal Number of decimal places to remain
 * @return string Return the number as a string
 */
string truncDecimal(double num, int decimal) {
    string strNum = to_string(num);
    size_t dot = strNum.find(".", 0, 1);
    return strNum.substr(0, dot + decimal + 1);
}

/**
 * @brief Method taken from next.c; reads packets and populates a structure
 * @param fd an open file to read packets from
 * @param pinfo allocated memory to put packet info into for one packet
 * @return unsigned short VALID_PKT (1) if a packet was read and pinfo is setup for processing the packet & 
 *                        NO_PACKET (0) if we have hit the end of the file and no packet is available 
 */
unsigned short nextPacket (int fd, struct pkt_info *pinfo, struct meta_info *meta) {
    int bytesRead; // bytes read
    // Set memories & initialize fields to null
    memset(pinfo, 0x0, sizeof(struct pkt_info));
    memset(meta, 0x0, sizeof(struct meta_info));

    // read the meta information
    bytesRead = read(fd, &meta, sizeof(struct meta_info)); // Read fd (file descriptor) into meta
    if (bytesRead == 0) // End of file reached
        return NO_PACKET;
    if (bytesRead < sizeof(meta_info)) // Smaller number of bytes read than expected
        errorExit("cannot read meta information", nullptr);
    pinfo->caplen = ntohs(meta->caplen); // Trace file; only the length of the packet portion
    pinfo->now = ntohl(meta->secs) + (double)((ntohl)(meta->usecs))/1000000; // Timestamp based on meta.secs & meta.usecs

    // Return if the packet is empty or erroneous based on length
    if (pinfo->caplen == 0) // Packet's length equals 0; nothing after meta information
        return VALID_PKT;
    if (pinfo->caplen > MAX_PKT_SIZE) // Packet is too big
        errorExit("packet too big", nullptr); // FIX; maybe loop implementation?

    // read the packet contents
    bytesRead = read(fd, pinfo->pkt, pinfo->caplen); // into pinfo's pkt field
    if (bytesRead < 0) // Error occurred
        errorExit("error reading packet", nullptr);
    if (bytesRead < pinfo->caplen) // Length smaller than expected; not enough info present
        errorExit("unexpected end of file encountered", nullptr);
    
    // Ethernet header
    if (bytesRead < sizeof(struct ether_header)) // No valid ethernet header
        return VALID_PKT;
    pinfo->ethh = (struct ether_header*)pinfo->pkt; // Point to ethernet header
    pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type); // Byte-order conversion
    if (pinfo->ethh->ether_type != ETHERTYPE_IP) // Check for Non-IP packets; nothing more to do
        return VALID_PKT;
    if (pinfo->caplen == sizeof(struct ether_header)) // Nothing beyond the ethernet header to process
        return VALID_PKT;

    // IP header
    pinfo->iph = (struct iphdr*)(pinfo->pkt + sizeof(struct ether_header)); // Point to the start of IP header
    size_t upToIP = sizeof(struct ether_header) + pinfo->iph->tot_len; // Length up to the end of IP header
    if (bytesRead < upToIP) // Check if the IP header is valid
        return VALID_PKT;
    // More byte-order conversions
    pinfo->iph->tot_len = ntohs(pinfo->iph->tot_len); // Total length; byte-order converted
    pinfo->iph->saddr = ntohl(pinfo->iph->saddr);
    pinfo->iph->daddr = ntohl(pinfo->iph->daddr);
    pinfo->iph->protocol = ntohs(pinfo->iph->protocol);
    pinfo->iph->id = ntohs(pinfo->iph->id);
    pinfo->iph->ttl = ntohs(pinfo->iph->ttl);
    // Check if there's no headers after ip
    if (sizeof(struct ether_header) + (pinfo->iph->tot_len) == pinfo->caplen) // There's no more after IP header
        return VALID_PKT;

    // TCP/UCP header
    if (pinfo->iph->protocol == IPPROTO_TCP) { // TCP protocol
        pinfo->tcph = (struct tcphdr*)(pinfo->pkt + upToIP); // Beginning of the TCP header
        // Byte-order conversions
        pinfo->tcph->source = ntohs(pinfo->tcph->source);
        pinfo->tcph->dest = ntohs(pinfo->tcph->dest);
        pinfo->tcph->ack_seq = ntohl(pinfo->tcph->ack_seq);
        pinfo->tcph->window = ntohs(pinfo->tcph->window);
    }
    else if (pinfo->iph->protocol == IPPROTO_UDP) { // UDP protocol
        pinfo->udph = (struct udphdr*)(pinfo->pkt + upToIP); // Beginning of the UDP header
        // Byte-order conversions
        pinfo->udph->source = ntohs(pinfo->udph->source);
        pinfo->udph->dest = ntohs(pinfo->udph->dest);
        pinfo->udph->len = ntohs(pinfo->udph->len);
    }
    return VALID_PKT;
}

/**
 * @brief Provides a high-level summary of the trace file
 * @param fd File description; contains all packets
 */
void summaryMode(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information

    // Values to be returned
    bool firstPacket = true; // Indicator for first packet
    int first_time; // Time of the first packet in the trace file
    int last_time; // Time of the last packet in the trace file
    double trace_duration; // The duration of the trace file
    int total_pkts; // Number of total packets
    int ip_pkts; // Number of IP packets

    // Iterating through all packets
    while (nextPacket(fd, &pinfo, &meta) == VALID_PKT) {
        if (firstPacket) {
            first_time = pinfo.now;
            firstPacket = false;
        }
        else // Update for all non-first packets
            last_time = pinfo.now;
        total_pkts = total_pkts + 1; // Increment by 1 for each iteration/packet
        if (pinfo.iph != NULL)
            ip_pkts = ip_pkts + 1; // Increment if the current packet is an IP packet
    }
    trace_duration = last_time - first_time; // Calculated by taking the difference between the first/last times

    // Print out the values
    fprintf(stdout, "time: first: %s last: %s duration: %s\r\n", 
            DOUBLE_PRINT(first_time, PADDING), DOUBLE_PRINT(last_time, PADDING), 
            DOUBLE_PRINT(trace_duration, PADDING));
    fprintf(stdout, "pkts: total: %s ip: %s\r\n", 
            INT_PRINT(total_pkts), INT_PRINT(ip_pkts));
}

// Print length information about each IP packet in the packet trace file
// Ignore for non-IP packets or if ethernet header is not present
void lengthAnalysis(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information

    // Values to be returned
    double ts; // timestamp of the packet in meta info
    int caplen; // number of bytes captured from the original packet
    string ip_len; // total length of IP packet in bytes
    string iphl; // total length of IP header
    string transport; // indicates which transport protocol
    string trans_hl; // total number of bytes occupied by transport header; ASK
    string payload_len; // number of app. layer payload bytes

    // Iterating through all packets
    while (nextPacket(fd, &pinfo, &meta) == VALID_PKT) {
        if (pinfo.ethh == nullptr || pinfo.ethh->ether_type != ETHERTYPE_IP) // No ethernet header or non-IP packet
            return;
        ts = pinfo.now;
        caplen = pinfo.caplen;
        if (pinfo.iph != nullptr) { // If IP header exists
            ip_len = to_string(pinfo.iph->tot_len);
            iphl = to_string((pinfo.iph->ihl) * BYTE);
        }
        else {
            ip_len = "-";
            iphl = "-";
            transport = "-";
            trans_hl = "-";
            payload_len = "-";
        }
        if (pinfo.iph->protocol == IPPROTO_TCP) { // If TCP indicated in IP header
            transport = "T";
            if (pinfo.tcph != nullptr) { // If TCP header exists
                trans_hl = to_string(pinfo.tcph->doff * BYTE);
                payload_len = stoi(ip_len) - ((stoi)(trans_hl) + (stoi)(iphl));
            }
            else {
                trans_hl = "-";
                payload_len = "-";
            }
        }
        else if (pinfo.iph->protocol != IPPROTO_UDP) { // If UDP indicated in IP header
            transport = "U";
            if (pinfo.udph != nullptr) { // If UDP header exists
                trans_hl = sizeof(struct udphdr);
                payload_len = stoi(ip_len) - ((stoi)(trans_hl) + (stoi)(iphl));
            }
            else {
                trans_hl = "-";
                payload_len = "-";
            }
        }
        else { // Non-TCP/UDP protocol specified
            transport = "?";
            trans_hl = "?";
            payload_len = "?";
        }
        
        // Output for each IP packet
        fprintf(stdout, "%s %s %s %s %s %s %s",
                DOUBLE_PRINT(ts, PADDING), INT_PRINT(caplen), ip_len.c_str(), 
                iphl.c_str(), transport.c_str(), trans_hl.c_str(), payload_len.c_str());
    }
}

// Output a single line of info about each TCp packet
// Non-TCP packets or packets w/o TCP header are ignored
void packetPrinting(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information

    // Values to return
    double ts; // timestamp of the packet
    double src_ip; // source IP address
    double src_port; // TCP's source port number
    double dst_ip; // destination IP address
    double dst_port; // TCP's destination port number
    double ip_id; // IP's ID field
    double ip_ttl; // IP's TTL field
    double window; // TCP's advertised window field
    double ackno; // TCP's ack. number field in ACK packets

    // Output for each TCP packet
    fprintf(stdout, "%s %s %s %s %s %s %s %s",
            ts, src_ip, src_port, dst_port, ip_id, ip_ttl, window, ackno);
}

// Keep track of the number of packets & total amt. of app. layer data from host to each peers using TCP
// Non-TCP packets ignored
void packetCounting(int fd) {
    // src_ip: IP address that sends the packets; dotted-quad notation
    double src_ip;
    // dst_ip: IP address that receives the packets; dotted-quad notation
    double dst_ip;
    // total_pkts: decimal rep. of total # of TCP packets from src_ip to dst_ip
    int total_pkts;
    // traffic_volume: decimal rep. of total # app. layer over TCP across all packets
    int traffic_volume;

    // Output a line for each (src, dst) pair
    fprintf(stdout, "%s %s %s %s\r\n", src_ip, dst_ip, total_pkts, traffic_volume);
}

/**
 * @brief The main method that takes in the input arguments and runs the program
 * @param argc Number of arguments
 * @param argv Arguments
 * @return int Return value of the main method
 */
int main(int argc, char* argv[]) {
    tuple<string, string> info = parseArgs(argc, argv); // When returned, all required inputs present (at least number-wise)   
    string mode = get<0>(info); // Save returned mode
    string traceFile = get<1>(info); // Save returned name of trace file
   
    // Determine which mode from argument parsed above
    int fd = open(traceFile.c_str(), O_RDONLY);
    if (fd < -1)
        errorExit("Error occurred while opening trace file %s", traceFile.c_str());
    
    // Call the appropriate methods according to the mode
    if (mode == "-s")
        summaryMode(fd);
    else if (mode == "-l")
        lengthAnalysis(fd);
    else if (mode == "-p")
        packetPrinting(fd);
    else if (mode == "-c")
        packetCounting(fd);
}