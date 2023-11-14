# Name: Chaehyeon Kim
# CWRU ID: cxk445
# File name: packetTraceAnalysis.py
# Date created: 11/11/2023
# Description: For the extra credit portion of CSDS 325 project 4; analyze different headers' lengths

import os
import statistics
import pandas as pd

# Contains appropriate index for each type
ind = {
    "caplen": 1,    # captured length
    "IP": 3,        # IP header length
    "TType": 4,     # Transport protocol
    "Transport": 5, # TCP/UDP header length
    "Payload": 6    # Payload length
}

# Opening the text file containing the output to the -l mode
def openFile():
    curr_dir = os.path.dirname(__file__) # Directory to the current script
    file_path = "ExtraCredit/l425.txt" # File from curr_dir to file
    abs_file_path = os.path.join(curr_dir, file_path) # Absolute path of the file
    file = open(abs_file_path, "r") # Contains the output
    return file

# Iterating through the output file and calculating appropriate values
def iterateOutput(file):
    # Total sum of all lengths of each header/caplen,etc
    totalCaplen = totalIPHeader = totalTCPHeader = totalUDPHeader = totalPayload = 0 # all initialized to 0
    # Total number of each type of packets
    numPkt = numTCP = numUDP = numApp = 0 # all initialized to 0
    #Lists containing all values across the packets
    caplenList, ipList, tcpList, udpList, payloadList = ([] for list in range(5))

    # Iterate through each line
    for line in file: 
        numPkt += 1 # Increment the number of total packet
        line = line.strip("\n") # Get rid of the '\n'
        splitLine = line.split(" ") # Split each line by whitespace

        # Modify appropriate values to variables above
        # Captured length
        caplen = int(splitLine[ind["caplen"]]) # Packet length
        totalCaplen += caplen
        caplenList.append(caplen)

        # IP header
        iph = int(splitLine[ind["IP"]]) # IP header length
        totalIPHeader += iph
        ipList.append(iph)

        # TCP header
        if (splitLine[ind["TType"]] == "T" 
            and splitLine[ind["Transport"]] != "?" and splitLine[ind["Transport"]] != "-"): # Has a TCP header
            tcph = int(splitLine[ind["Transport"]])
            totalTCPHeader += tcph
            numTCP += 1
            tcpList.append(tcph)

        # UDP header
        elif (splitLine[ind["TType"]] == "U" 
              and splitLine[ind["Transport"]] != "?" and splitLine[ind["Transport"]] != "-"): # Has a UDP header
            udph = int(splitLine[ind["Transport"]])
            totalUDPHeader += udph
            numUDP += 1
            udpList.append(udph)

        # Payload
        if (splitLine[ind["Payload"]] != "?" and splitLine[ind["Payload"]] != "-"): # Has a payload value
            payload = int(splitLine[ind["Payload"]])
            totalPayload += payload
            numApp += 1
            payloadList.append(payload)

    # Store the mean/median of each packet/header/payload lengths
    averages = {
        "Packet": [totalCaplen / numPkt, statistics.median(caplenList)],
        "IP header": [totalIPHeader / numPkt, statistics.median(ipList)],
        "TCP header": [totalTCPHeader / numTCP, statistics.median(tcpList)],
        "UDP header": [totalUDPHeader / numUDP, statistics.median(udpList)],
        "Payload": [totalPayload / numApp, statistics.median(payloadList)]
    }
    
    # Create a data frame from the data above & print it
    df = pd.DataFrame(averages)
    df.index = ["Mean", "Median"]
    print(df)
    
# Main method
if __name__ == '__main__':
    file = openFile() # Open the -l output file
    iterateOutput(file)
    file.close() # Closing the -l output file

    