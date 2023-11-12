/**
* @file proj4.cpp
* @author Chaehyeon Kim (cxk445)
* @brief Script for CSDS 325 HW4; working with packets
* @date 2023-10-25
*/

#include <string>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> 
#include <unordered_map>

using namespace std;

/* Defining macros */
#define EMPTY 0x0
#define ERROR 1
#define ACK 1
#define SHIFT 1
#define INITIAL -1
#define ARG_LENGTH 4
#define MAX_PKT_SIZE 1600
#define VALID_PKT 1
#define NO_PACKET 0
#define NO_PADDING 0
#define PADDING 6
#define BYTE 4
#define META_INFO_BYTES 12
#define META 1
#define ETHER 2
#define IP 3
#define TCP 4
#define UDP 5

// Comparing arguments case-insensitive 
#define COMPARE_ARG(arg, opt) (0 == strncasecmp(arg, opt, strlen(arg)))
// Converting a double number to const char to print
#define INT_PRINT(num) (to_string(num).c_str())
// Converting an int number to const char to print
#define DOUBLE_PRINT(num, dec) (truncDecimal(num, dec).c_str())

/* Defining structs */

// meta information, using same layout as trace file
struct meta_info {
    unsigned int usecs; // microseconds; 10^-6 of a second
    unsigned int secs; // seconds
    unsigned short ignored;
    unsigned short caplen; // Amount of data available in the packet portion (doesn't include meta-info)
};

// record of information about the current packet
struct pkt_info {
    unsigned short caplen;           // captured length; from meta info
    double now;                      // time combining usecs and secs from meta info
    unsigned char pkt[MAX_PKT_SIZE]; // Stores the whole packet after meta-info
    struct ether_header *ethh;       // ptr to ethernet header, if present, otherwise NULL
    struct iphdr *iph;               // ptr to IP header, if present, otherwise NULL
    struct tcphdr *tcph;             // ptr to TCP header, if present, otherwise NULL
    struct udphdr *udph;             // ptr to UDP header, if present, otherwise NULL
};

// source/destination IP address pairs
struct source_dest_key {
    string sourceIP; // Source IP address
    string destIP; // Destination IP address
    
    // Overriding '==' operator to compare source/dest pairs
    bool operator==(const source_dest_key &otherPair) const{
        return (sourceIP == otherPair.sourceIP && destIP == otherPair.destIP); // Both source/dest equal
    }
};

// Stores the appropriate values for each source/dest. IP address pairs
struct source_dest_value {
    int traffic_volume = EMPTY; // Total number of application layer bytes; initialized to 0
    int total_pkts = EMPTY; // Total number of packets; initialized to 0
};

// Defining hash function for pairs of source/dest. IP addresses
namespace std {
    template<> struct hash<source_dest_key> {
        size_t operator()(const source_dest_key &pair) const {
            using std::size_t;
            using std::hash;
            using std::string;
            return ((hash<string>()(pair.sourceIP) ^ 
                    (hash<string>()(pair.destIP) << SHIFT)) >> SHIFT);
    }
};}

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
    int indexR = INITIAL; // Saves the index of the "-r" argument; initialized to -1
    bool optDetermined = false; // Indicates whether mode determined or not; prevent multiple modes
    string mode = "";
    string traceFile = "";
    for (int i = 1; i < argc; i++) { // Only 3 arguments must exist; -r, trace_file, and mode specification
        const char* arg = argv[i];
        if (COMPARE_ARG(arg, "-r") && indexR == INITIAL) // arg is "-r" && "-r" not found yet 
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
        else if (indexR != INITIAL && i == indexR + 1 && !COMPARE_ARG(arg, "-r")) // "-r" was the previous argument; trace file
            traceFile = arg;
        else
            errorExit("Invalid, duplicate, or out-of-order argument %s", arg);
    }
    if (argc != ARG_LENGTH || indexR == INITIAL || traceFile.empty() || !optDetermined) // Specifications not all present 
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
 * @brief Converts the input IP address and convert to dotted-quad form
 * @param ipAddress IP address to be converted
 * @return string IP address in dotted-quad form
 */
string dottedQuadConversion(uint32_t ipAddress) {
    struct in_addr addr;
    addr.s_addr = ipAddress;
    return inet_ntoa(addr);
}

/**
 * @brief Convert byte orders for fields to be used later 
 * @param part Specifies the part of the packet; meta-info, ethernet, IP, TCP/UDP
 * @param pinfo Contains information about the packet
 * @param meta Contains meta information
 */
void convertByteOrders(int part, struct pkt_info *pinfo, struct meta_info *meta) {
    if (part == META) { // Meta
        pinfo->caplen = ntohs(meta->caplen); // Trace file; only the length of the packet portion
        pinfo->now = ntohl(meta->secs) + (double)((ntohl)(meta->usecs))/1000000; // Timestamp based on meta.secs & meta.usecs
    }
    if (part == ETHER) {
        pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type); // Byte-order conversion
    }
    if (part == IP) { // IP header
        pinfo->iph->tot_len = ntohs(pinfo->iph->tot_len); // Total length; byte-order converted
        pinfo->iph->ihl = ntohs(pinfo->iph->ihl);
        pinfo->iph->saddr = ntohl(pinfo->iph->saddr);
        pinfo->iph->daddr = ntohl(pinfo->iph->daddr);
        pinfo->iph->protocol = ntohs(pinfo->iph->protocol);
        pinfo->iph->id = ntohs(pinfo->iph->id);
        pinfo->iph->ttl = ntohs(pinfo->iph->ttl);
    }
    else if (part == TCP) { // TCP
        pinfo->tcph->source = ntohs(pinfo->tcph->source);
        pinfo->tcph->dest = ntohs(pinfo->tcph->dest);
        pinfo->tcph->ack = ntohl(pinfo->tcph->ack);
        pinfo->tcph->ack_seq = ntohl(pinfo->tcph->ack_seq);
        pinfo->tcph->window = ntohs(pinfo->tcph->window);
        pinfo->tcph->doff = ntohs(pinfo->tcph->doff);
    }
    else if (part == UDP) {
        pinfo->udph->source = ntohs(pinfo->udph->source);
        pinfo->udph->dest = ntohs(pinfo->udph->dest);
        pinfo->udph->len = ntohs(pinfo->udph->len);
    }
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
    bytesRead = read(fd, meta, sizeof(struct meta_info)); // Read fd (file descriptor) into meta
    if (bytesRead == 0) // End of file reached
        return NO_PACKET;
    if (bytesRead < sizeof(meta_info)) // Smaller number of bytes read than expected
        errorExit("cannot read meta information", nullptr);
    convertByteOrders(META, pinfo, meta);
   
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
    convertByteOrders(ETHER, pinfo, meta);
    if (pinfo->ethh->ether_type != ETHERTYPE_IP) // Check for Non-IP packets; nothing more to do
        return VALID_PKT;
    if (pinfo->caplen == sizeof(struct ether_header)) // Nothing beyond the ethernet header to process
        return VALID_PKT;

    // IP header
    pinfo->iph = (struct iphdr*)(pinfo->pkt + sizeof(struct ether_header)); // Point to the start of IP header
    size_t upToIP = sizeof(struct ether_header) + pinfo->iph->tot_len; // Length up to the end of IP header
    if (bytesRead < upToIP) // Check if the IP header is valid
        return VALID_PKT;
    convertByteOrders(IP, pinfo, meta); // Byte-order conversions for applicable fields
    // Check if there's no headers after ip
    if (sizeof(struct ether_header) + (pinfo->iph->tot_len) == pinfo->caplen) // There's no more after IP header
        return VALID_PKT;

    // TCP/UCP header
    if (pinfo->iph->protocol == IPPROTO_TCP) { // TCP protocol
        pinfo->tcph = (struct tcphdr*)(pinfo->pkt + upToIP); // Beginning of the TCP header
        convertByteOrders(TCP, pinfo, meta);
    }
    else if (pinfo->iph->protocol == IPPROTO_UDP) { // UDP protocol
        pinfo->udph = (struct udphdr*)(pinfo->pkt + upToIP); // Beginning of the UDP header
        convertByteOrders(UDP, pinfo, meta);
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
    bool firstPacket = true; // Indicator for first packet
    double first_time; // Time of the first packet in the trace file
    double last_time; // Time of the last packet in the trace file
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
        total_pkts += 1; // Increment by 1 for each iteration/packet
        if (pinfo.ethh->ether_type == ETHERTYPE_IP)
            ip_pkts += 1; // Increment if the current packet is an IP packet
    }
    trace_duration = last_time - first_time; // Calculated by taking the difference between the first/last times

    // Print out the values
    fprintf(stdout, "time: first: %s last: %s duration: %s\r\n", 
            DOUBLE_PRINT(first_time, PADDING), DOUBLE_PRINT(last_time, PADDING), 
            DOUBLE_PRINT(trace_duration, PADDING));
    fprintf(stdout, "pkts: total: %s ip: %s\r\n", 
            INT_PRINT(total_pkts), INT_PRINT(ip_pkts));
}

/**
 * @brief Print length information about each IP packet in the packet trace file
 * @param fd File descriptor
 */
void lengthAnalysis(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information
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
            continue;
        ts = pinfo.now;
        caplen = pinfo.caplen;
        if (pinfo.iph != nullptr) { // If IP header exists
            ip_len = to_string(pinfo.iph->tot_len);
            iphl = to_string((pinfo.iph->ihl) * BYTE);
        }
        else { // IP header is not present
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
                trans_hl = sizeof(struct udphdr); // Fixed header length
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
        fprintf(stdout, "%s %s %s %s %s %s %s\r\n",
                DOUBLE_PRINT(ts, PADDING), INT_PRINT(caplen), ip_len.c_str(), 
                iphl.c_str(), transport.c_str(), trans_hl.c_str(), payload_len.c_str());
    }
}

/**
 * @brief Output a single line of info about each TCP packet with header
 * @param fd File descriptor
 */
void packetPrinting(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information
    double ts; // timestamp of the packet
    string src_ip; // source IP address
    string dst_ip; // destination IP address
    double src_port; // TCP's source port number
    double dst_port; // TCP's destination port number
    double ip_id; // IP's ID field
    double ip_ttl; // IP's TTL field
    double window; // TCP's advertised window field
    string ackno; // TCP's ack. number field in ACK packets

    // Iterating through all packets
    while (nextPacket(fd, &pinfo, &meta) == VALID_PKT) {
        if (pinfo.tcph == nullptr) // Non-TCP packets or TCP packets without headers
            continue;
        ts = pinfo.now; // Timestamp
        src_ip = dottedQuadConversion(pinfo.iph->saddr); // In dotted-quad form
        dst_ip = dottedQuadConversion(pinfo.iph->daddr); // In dotted-quad form
        src_port = pinfo.tcph->source;
        dst_port = pinfo.tcph->dest; 
        ip_id = pinfo.iph->id;
        ip_ttl = pinfo.iph->ttl;
        window = pinfo.tcph->window;
        if (pinfo.tcph->ack == ACK) // ACK bit set to 1
            ackno = to_string(pinfo.tcph->ack_seq);
        else
            ackno = "-";

        // Output for each TCP packet
        fprintf(stdout, "%s %s %s %s %s %s %s %s %s\r\n",
                DOUBLE_PRINT(ts, PADDING), 
                src_ip.c_str(), DOUBLE_PRINT(src_port, NO_PADDING), 
                dst_ip.c_str(), DOUBLE_PRINT(dst_port, NO_PADDING), 
                DOUBLE_PRINT(ip_id,NO_PADDING), DOUBLE_PRINT(ip_ttl, NO_PADDING), 
                DOUBLE_PRINT(window, NO_PADDING), ackno.c_str());
    }
}

/**
 * @brief Keep track of the number of packets & total amt. of app. layer data from host to each peers using TCP
 * @param fd File descriptor
 */
void packetCounting(int fd) {
    struct pkt_info pinfo; // To contain packet information
    struct meta_info meta; // To contain meta information
    unordered_map<source_dest_key, source_dest_value> transactions = {}; // Hash map for storing info about each pairs

    // Iterating through all packets
    while (nextPacket(fd, &pinfo, &meta) == VALID_PKT) {
        if (pinfo.tcph == nullptr) // Non-TCP packets or TCP packets without headers; skip current packet
            continue;
        string source = dottedQuadConversion(pinfo.iph->saddr); // IP address that sends the packets
        string dest = dottedQuadConversion(pinfo.iph->daddr); // IP address that receives the packets
        int trafficVolume = pinfo.iph->tot_len - ((pinfo.tcph->doff * BYTE) + ((pinfo.iph->ihl) * BYTE));
    
        //input the values into the hash table
        transactions[{source, dest}].traffic_volume += trafficVolume; // Append current traffic volume
        transactions[{source, dest}].total_pkts += 1;        
    }
    // Iterating through the hash map to print out values
    for (const auto &[key, value] : transactions) {
        // Output a line for each (src, dst) pair
        fprintf(stdout, "%s %s %s %s\r\n", 
                key.sourceIP.c_str(), key.destIP.c_str(), 
                INT_PRINT(value.total_pkts), INT_PRINT(value.traffic_volume));
    }
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
    
    // Close the trace file
    close(fd); 
}