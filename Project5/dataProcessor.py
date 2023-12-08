# Name/Case ID: Chaehyeon Kim (cxk445)
# File name: dataProcessor.py
# Description: For CSDS 325 Project 5; gathering measurements for different networks
# Date: 11/25/2023

import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt 

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
    rttInfo = [] # To include provided information about RTTs
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
        # Save the additional RTT info provided at the end
        rttLine = lines[totalLines - 1]
        rttInd = rttLine.find("= ") + len("= ")
        rttEnd = rttLine.find(" ms")
        rttInfo.append(rttLine[rttInd: rttEnd].split("/"))
    return rttInfo

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
        while (traceN < 900): # FIX to 1000
            line = lines[lineIt]
            if (line.find("traceroute to") != -1): # If new traceroute command started
                if (lineIt != 0): # If not the first line
                    dataframe.iloc[traceN, 3].append(maxHop)
                    traceN += 1
            else: # Still traversing through the same traceroute command results
                numTime = int(line[0: 2]) # Number at the front of the line
                if (line.find("* * *") == -1 or (numTime == 30 and line.find("* * *") != -1)): # Valid step or timeout
                    maxHop = numTime
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

# Creating visual representations for RTT data
def plotRTT(network, dataframe, rttInfo):
    # Setting up the labels
    plt.title(f"Round Trip Time of {network}")
    plt.xlabel("ith Ping Request")
    plt.ylabel("RTT in ms")
    # Plotting for each website by iterating through data
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:pink', 'tab:purple', 'tab:olive'] # Color map
    for website in range(len(websiteList)): # For each website
        x = []
        y = []
        for i in range(1000): # For all rows of data
            x.append(i)
            y.append(dataframe.iloc[i, 0][website]) # RTT value
        plt.plot(x, y, label=f"{websiteList[website]}", color=colors[website])
    plt.legend()
    plt.show()
    # Creating a table containing statistical values of RTT values
    valueTypes = ["Minimum", "Average", "Maximum", "Standard Deviation"]
    plt.table(cellText=rttInfo, colLabels=valueTypes, rowLabels=websiteList, loc='center')
    plt.axis('off')
    plt.show()

def plotPacketLoss(network, dataframe):
    # Creating the list to create the table with
    packetsSent = [1000] * len(websiteList)
    packetsLost = dataframe.iloc[0, 1]
    for i in range(len(packetsLost)): # Converting values from string to int
        packetsLost[i] = int(packetsLost[i])
    percentage = []
    for i in range(len(websiteList)): # Calculating the percentage with the packets sent/lost
        percentage.append(packetsLost[i] / packetsSent[i])
    data = [packetsLost, packetsSent, percentage]
    # Creating a table containing information about packet loss
    valueTypes = ["Packets Lost", "Packets Sent", "Percentage of Packet Loss"]
    plt.table(cellText=data, rowLabels=valueTypes, colLabels=websiteList, loc='center')
    plt.title(f"Packet Loss of {network}")
    plt.axis('off')
    plt.show()

def plotJitter(network, dataframe):
    print()

def plotHops(network, dataframe):
    print()

def plotThroughput(network, dataframe):
    print()

def plotBandwidth(network, dataframe):
    print()

def plotRetransmission(network, dataframe):
    print()




# The main function
if __name__ == '__main__':
    # Initializing the dataframes for both Spectrum and CaseWireless
    columns = ["RTT", "Packet Loss", "Jitter", "Number of Hops", "Throughput", "Bandwidth", "Retransmission"] # Types of measurements
    spectrumData = pd.DataFrame(index=np.arange(1000), columns=columns) # Number of rows preset to 1000 & columns as measurements
    CWData = pd.DataFrame(index=np.arange(1000), columns=columns) # Same as above but for CaseWireless data
    setEmptyLists([spectrumData, CWData])

    # Iterating through the data and adding to the appropriate dataframes
    for network in ["Spectrum", "CaseWireless"]:
        df = spectrumData if (network == "Spectrum") else CWData # Selecting the correct dataframe
        # Process the data files
        rttInfo = processPingData(network, df)
        processIPerfData(network, df)
        processTraceRouteData(network, df)
        processNetstatData(network, df)

        # Creating plots, tables, etc. for data representation
        # plotRTT(network, df, rttInfo)
        plotPacketLoss(network, df)
        plotJitter(network, df)
        plotHops(network, df)
        plotThroughput(network, df)
        plotBandwidth(network, df)
        plotRetransmission(network, df)
    