# Name/Case ID: Chaehyeon Kim (cxk445)
# File name: dataProcessor.py
# Description: For CSDS 325 Project 5; gathering measurements for different networks
# Date: 11/25/2023

import os
import pandas as pd
import numpy as np

# A list of websites the network traffics lead to
websiteList = ["Amazon", "Canvas", "Case", "Google", "Instagram", "Youtube"]

# A function to open the specified file containing data
def openFile(fileType, destName="", network=""):
    folderDir = os.path.dirname(__file__) + "/" + network # Directory to the current script
    fileName = fileType + destName + ("CW" if network=="CaseWireless" else "") + ".txt"
    absFileDir = os.path.join(folderDir, fileName) # Absolute path of the file
    file = open(absFileDir, "r") # Contains the output
    lines = file.readlines()
    return lines

# Set each entry of the specified columns to empty lists
def setEmptyLists(dataframes):
    for df in dataframes:
        df["RTT"] = np.empty((len(spectrumData), 0)).tolist()
        df["Packet Loss"].iloc[0] = list() # Just need one for overall summary
        df["Jitter"] = np.empty((len(spectrumData), 0)).tolist()
        df["Number of Hops"] = np.empty((len(spectrumData), 0)).tolist()

# Process the ping data files for all 6 sites, each for Spectrum and CaseWireless
def processPingData(network, dataframe):
    # Go through files for all websites
    for website in websiteList:
        lines = openFile("ping", website, network) # Open the appropriate file
        totalLines = len(lines)
        prevRTT = -1
        # Iterating through each lines except beginning & summary
        for i in range(1, totalLines-4):
            line = lines[i] # Current line
            # RTT
            rttIndex = line.find("time=") + 5 # Index of where RTT value starts
            rttEnd = line.find("ms") # Index of where RTT value ends
            rtt = float(line[rttIndex: rttEnd])
            dataframe.iloc[i-1, 0].append(rtt) # Add RTT value to dataframe
            # Jitter
            jitter = abs(prevRTT - rtt) if (i != 1) else 0 # Jitter 0 if no prev. RTT
            dataframe.iloc[i-1, 2].append(jitter) # Adding calculated jitter
            prevRTT = rtt
        # Fill in for missing data/packets (add None as placeholder)
        for i in range(1000 - (totalLines - 5)):
            dataframe.iloc[999-i, 0].append(None)
            dataframe.iloc[999-i, 2].append(None)
        # Add the website's packet loss rate
        summaryLine = lines[totalLines - 2]
        recIndex = summaryLine.find("transmitted,") + len("transmitted,")
        recEnd = summaryLine.find("received")
        dataframe.iloc[0, 1].append(summaryLine[recIndex: recEnd])

# Process the iperf data for each network 
def processIPerfData(network, dataframe):
    lines = openFile("iperf", network=network) # File containing iperf data
    # Iterate through all lines after basic information
    for i in range(6, len(lines)-1):
        line = lines[i]
        # Throughput
        thIndex = line.find("sec") + len("sec")
        thEnd = line.find("GBytes") 
        throughput = float(line[thIndex: thEnd])
        dataframe.iloc[i-6, 4] = throughput # Save for the nth iteration
        # Bandwidth
        bwIndex = line.find("GBytes") + len("GBytes")
        bwEnd = line.find("Gbits/sec") 
        bandwidth = float(line[bwIndex: bwEnd])
        dataframe.iloc[i-6, 5] = bandwidth # Save for the nth iteration
        
# Process the traceroute data for each website
def processTraceRouteData(network, dataframe):
    for website in websiteList:
        lines = openFile("tr", website, network) # traceroute file
        lineIt = 0 # Total number of lines iterated
        maxHop = 0 # Max number of hops for each traceroute command
        traceN = 0 # The nth iteration of separate traceroute commands
        # Traverse the data until 1000 entries are filled
        while (traceN < 1000):
            line = lines[lineIt]
            if (line.find("traceroute to") != -1): # If new traceroute command started
                if (lineIt != 0):
                    dataframe.iloc[traceN, 3].append(maxHop)
                    traceN += 1
                maxHop = 0
            else: # Still traversing through the same traceroute command results
                if (line.find("* * *") == -1):
                    maxHop = int(line[0: 2])
            lineIt += 1

# Process netstat data for both networks
def processNetstatData(network, dataframe):
    lines = openFile("netstat", network=network) # Opening the netstat file
    totalSent = -1 # Total number of TCP packets sent
    totalResend = -1 # Total number of retransmitted TCP packets
    # Start at an arbitrary line to save time
    for i in range(20, len(lines)):
        line = lines[i]
        # See if the current line contains desired values
        totalSentInd = line.find("segments sent out")
        totalResendInd = line.find("segments retransmitted")
        if (totalSentInd > 0): # Contains total # sent packets
            totalSent = int(line[0: totalSentInd])
        elif (totalResendInd > 0): # Contains total # retransmission
            totalResend = int(line[0: totalResendInd])
            break # No longer need to traverse the lines
    retransmissionRate = totalResend / totalSent # Calculating retransmission rate
    dataframe.iloc[0, 6] = [retransmissionRate, totalResend, totalSent]

# The main function
if __name__ == '__main__':
    # Initializing the dataframes for both Spectrum and CaseWireless
    columns = ["RTT", "Packet Loss", "Jitter", "Number of Hops", "Throughput", "Bandwidth", "Retransmission"] # Types of measurements
    spectrumData = pd.DataFrame(index=np.arange(1000), columns=columns) # Number of rows preset to 1000 & columns as measurements
    CWData = pd.DataFrame(index=np.arange(1000), columns=columns) # Same as above but for CaseWireless data
    setEmptyLists([spectrumData, CWData])

    # Iterate through the data and add to the appropriate dataframes
    for network in ["Spectrum", "CaseWireless"]:
        df = spectrumData if (network == "Spectrum") else CWData # Selecting the correct dataframe
        processPingData(network, df)
        processIPerfData(network, df)
        processTraceRouteData(network, df)
        processNetstatData(network, df)