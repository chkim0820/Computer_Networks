# Name/Case ID: Chaehyeon Kim (cxk445)
# File name: dataProcessor.py
# Description: For CSDS 325 Project 5; gathering measurements for different networks
# Date: 11/25/2023

import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt 

# Global variables for constants
totalData = 1000 # Total number of data points
error = -1 # To initialize to -1; indicate error when equal after a specific number of iterations
rttSummary = 4 # Length of rttSummary
pingSummary = 5 # Length of ping summary
transSummary = 2 # Length of transmission summary
iperfIntro = 6 # Length of iPerf introduction
iperfSummary = 1 # Length of iPerf summary
hopLimit = 30 # The maximum limit for hops
numLen = 2 # Length of numbers indicating hops
numWebsites = 6 # Number fo all websites
overall = 1 # Data taken over all websites
avgCol = 1 # Number of the average column in ping summary
minCol = 0 # Number of the minimum column in ping summary
indexOne = 1 # For when the index starts count at 1
noJitter = 0 # Set the value of jitter to 0 when it's the first iteration
initial = 0 # For initializing to 0 or starting at index 0
empty = 0 # For indicating an empty value (0)

# A list of websites the network traffics lead to
websiteList = ["Amazon", "Canvas", "Case", "Google", "Instagram", "Youtube"]
# Dictionary to full domains of each website name
fullDomain = {"Amazon": "amazon.com", 
              "Canvas": "canvas.case.edu", 
              "Case": "case.edu", 
              "Google": "google.com", 
              "Instagram": "instagram.com", 
              "Youtube": "youtube.com"}
# A list of types of measurements (7 total)
measurements = ["Round Trip Time", "Packet Loss", "Jitter", "Number of Hops", "Throughput", "Bandwidth", "Retransmission"]
# Color map for plotting 6 different websites
colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:pink', 'tab:purple', 'tab:olive']



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
        df["Round Trip Time"] = np.empty((len(spectrumData), empty)).tolist()
        df["Packet Loss"].iloc[initial] = list() # Just need one for overall summary
        df["Jitter"] = np.empty((len(spectrumData), empty)).tolist()
        df["Number of Hops"] = np.empty((len(spectrumData), empty)).tolist()

# Process the ping data files for all 6 sites, each for Spectrum and CaseWireless
def processPingData(network, dataframe):
    rttInfo = [] # To include provided information about RTTs
    rttOrder = measurements.index("Round Trip Time")
    jitterOrder = measurements.index("Jitter")
    lossOrder = measurements.index("Packet Loss")
    # Go through files for all websites
    for website in websiteList:
        lines = openFile("ping", website, network) # Open the appropriate file
        totalLines = len(lines)
        prevRTT = error
        # Iterating through each lines except beginning & summary
        for i in range(indexOne, totalLines - rttSummary):
            line = lines[i] # Current line
            # RTT
            rttIndex = line.find("time=") + len("time=") # Index of where RTT value starts
            rttEnd = line.find("ms") # Index of where RTT value ends
            rtt = float(line[rttIndex: rttEnd])
            dataframe.iloc[i - indexOne, rttOrder].append(rtt) # Add RTT value to dataframe
            # Jitter
            jitter = abs(prevRTT - rtt) if (i != indexOne) else noJitter # Jitter 0 if no prev. RTT
            dataframe.iloc[i - indexOne, jitterOrder].append(jitter) # Adding calculated jitter
            prevRTT = rtt
        # Fill in for missing data/packets (add None as placeholder)
        for i in range(totalData - (totalLines - pingSummary)):
            dataframe.iloc[(totalData - indexOne) - i, rttOrder].append(None)
            dataframe.iloc[(totalData - indexOne) - i, jitterOrder].append(None)
        # Add the website's packet loss rate
        summaryLine = lines[totalLines - transSummary]
        recIndex = summaryLine.find("transmitted,") + len("transmitted,")
        recEnd = summaryLine.find("received")
        dataframe.iloc[initial, lossOrder].append(summaryLine[recIndex: recEnd])
        # Save the additional RTT info provided at the end
        rttLine = lines[totalLines - indexOne]
        rttInd = rttLine.find("= ") + len("= ")
        rttEnd = rttLine.find(" ms")
        rttInfo.append(rttLine[rttInd: rttEnd].split("/"))
    return rttInfo

# Process the iperf data for each network 
def processIPerfData(network, dataframe):
    lines = openFile("iperf", network=network) # File containing iperf data
    throughputOrder = measurements.index("Throughput")
    bandwidthOrder = measurements.index("Bandwidth")
    # Iterate through all lines after basic information
    for i in range(iperfIntro, len(lines) - iperfSummary):
        line = lines[i]
        # Throughput
        thIndex = line.find("sec") + len("sec")
        thEnd = line.find("GBytes") 
        throughput = float(line[thIndex: thEnd])
        dataframe.iloc[i - iperfIntro, throughputOrder] = throughput # Save for the nth iteration
        # Bandwidth
        bwIndex = line.find("GBytes") + len("GBytes")
        bwEnd = line.find("Gbits/sec") 
        bandwidth = float(line[bwIndex: bwEnd])
        dataframe.iloc[i - iperfIntro, bandwidthOrder] = bandwidth # Save for the nth iteration
        
# Process the traceroute data for each website
def processTraceRouteData(network, dataframe):
    maxHopOrder = measurements.index("Number of Hops")
    for website in websiteList:
        lines = openFile("tr", website, network) # traceroute file
        lineIt = initial # Total number of lines iterated
        maxHop = initial # Max number of hops for each traceroute command
        traceN = initial # The nth iteration of separate traceroute commands
        # Traverse the data until 1000 entries are filled
        while (traceN < totalData):
            line = lines[lineIt]
            if (line.find("traceroute to") != error): # If new traceroute command started
                if (lineIt != initial): # If not the first line
                    dataframe.iloc[traceN, maxHopOrder].append(maxHop)
                    traceN += 1
            else: # Still traversing through the same traceroute command results
                numTime = int(line[initial: numLen]) # Number at the front of the line
                if (line.find("* * *") == error or (numTime == hopLimit and line.find("* * *") != error)): # Valid step or timeout
                    maxHop = numTime
            lineIt += 1

# Process netstat data for both networks
def processNetstatData(network, dataframe):
    lines = openFile("netstat", network=network) # Opening the netstat file
    totalSent = error # Total number of TCP packets sent
    totalResend = error # Total number of retransmitted TCP packets
    # Start at an arbitrary line to save time
    for i in range(len(lines)):
        line = lines[i]
        # See if the current line contains desired values
        totalSentInd = line.find("segments sent out")
        totalResendInd = line.find("segments retransmitted")
        if (totalSentInd != error): # Contains total # sent packets
            totalSent = int(line[initial: totalSentInd])
        elif (totalResendInd != error): # Contains total # retransmission
            totalResend = int(line[initial: totalResendInd])
            break # No longer need to traverse the lines
    retransmissionRate = totalResend / totalSent # Calculating retransmission rate
    dataframe.iloc[initial, measurements.index("Retransmission")] = [retransmissionRate, totalResend, totalSent]

# Calculating minimum, maximum, average, and standard deviation of measurement data
def calculateStats(data, numCols):
    stats = [] # For saving values to be returned
    for col in range(numCols):
        values = [] # To store all entries for the column/website
        totalSum = initial # Total sum of all entries
        numRows = initial
        for row in range(len(data)):
            value = data.iloc[row][col] if (numCols==numWebsites) else data.iloc[row]
            if (value != None):
                values.append(value)
                totalSum += value
                numRows += 1
        avg = totalSum / numRows # FIX for diff number of valid packets
        minimum = min(values)
        maximum = max(values)
        sd = np.std(values)
        stats.append([avg, minimum, maximum, sd])
    return stats
        
# For plotting plots across all ping data
def plotPlots(network, data, measurement, unit, numCols=numWebsites, individual=False):
    plt.clf() # Clearing the plot
    # Setting up the labels
    plt.title(f"{measurement} of {network}")
    plt.xlabel(f"ith {measurement} Request")
    plt.ylabel(f"{measurement} in {unit}")
    # Plotting for each website by iterating through data
    for website in range(numCols): # For each website
        x = []
        y = []
        for i in range(len(data)): # For all rows of data
            x.append(i)       
            if (numCols == numWebsites): # Different values for each website
                y.append(data.iloc[i][website]) # measurement data
            elif (numCols == overall): # Values over all websites
                y.append(data[i]) # Contains float value; append the value
        siteName = websiteList[website]
        if (individual==True): # Show individual plots for each website
            plt.clf() # Clear the figure for other websites
            plt.title(f"{measurement} of {network} to {fullDomain[siteName]}")
            plt.xlabel(f"ith {measurement} Request")
            plt.ylabel(f"{measurement} in {unit}")
            plt.plot(x, y, color=colors[website])
            # plt.savefig(f"{network}_{siteName}_{measurement}_Plot")
            plt.show()
        else: # Add to the plot if not showing
            plt.plot(x, y, label=f"{siteName}", color=colors[website])
    plt.legend()
    # plt.savefig(f"{network}_{measurement}_Plot", bbox_inches="tight")
    plt.show()

# Creating a table with the input data
def plotTable(network, data, measurement, unit, numCols=numWebsites, dataParsed=False):
    plt.clf() # Clearing the plot
    plt.title(f"{measurement} of {network} in {unit}")
    valueTypes = ["Average", "Minimum", "Maximum", "Standard Deviation"]
    if (dataParsed == False):
        data = calculateStats(data, numCols) # Input number of columns (websites vs. overall)
    if (numCols==numWebsites):
        plt.table(cellText=data, colLabels=valueTypes, rowLabels=websiteList, loc='center')
    elif (numCols==overall):
        plt.table(cellText=[valueTypes, data[initial]], loc='center')
    plt.axis('off')
    # plt.savefig(f"{network}_{measurement}_Table", bbox_inches="tight")
    plt.show()

# Plotting plots and graphs for RTT
def plotRTT(network, dataframe, rttInfo):
    plotPlots(network, dataframe["Round Trip Time"], "Round Trip Time", "ms")
    # Shifting around columns (min, avg, max) to (avg, min, max)
    for row in rttInfo:
        avg = row[avgCol]
        row[avgCol] = row[minCol]
        row[minCol] = avg
    # plotTable(network, rttInfo, "Round Trip Time", "ms", dataParsed=True)

# Creating a table for packets lost
def plotPacketLoss(network, dataframe):
    plt.clf() # Clearing the plot
    # Creating the list to create the table with
    packetsSent = [totalData] * len(websiteList)
    packetsLost = dataframe.iloc[initial, measurements.index("Packet Loss")]
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
    # plt.savefig(f"{network}_PacketLoss_Table", bbox_inches="tight")
    plt.show()

# Plotting plots and graphs for jitter
def plotJitter(network, dataframe):
    data = dataframe["Jitter"].iloc[indexOne:len(dataframe)] # Exclude the first row
    plotPlots(network, data, "Jitter", "ms")
    # plotTable(network, data, "Jitter", "ms")

# Plotting plots and graphs for number of hops
def plotHops(network, dataframe):
    data = dataframe["Number of Hops"]
    plotPlots(network, data, "Number of Hops", "Total")
    # plotTable(network, data, "Number of Hops", "Total")

# Plotting plots and graphs for throughput
def plotThroughput(network, dataframe):
    data = dataframe["Throughput"]
    plotPlots(network, data, "Throughput", "GBytes", numCols=overall)
    # plotTable(network, data, "Throughput", "GBytes", numCols=overall)

# Plotting plots and graphs for bandwidth
def plotBandwidth(network, dataframe):
    data = dataframe["Bandwidth"]
    plotPlots(network, data, "Bandwidth", "Gbits/sec", numCols=overall)
    # plotTable(network, data, "Bandwidth", "Gbits/sec", numCols=overall)

# Plotting plots and graphs for retransmission rate
def plotRetransmission(network, dataframe):
    plt.clf() # Clearing the plot
    valueTypes = ["Number Retransmitted", "Total Packets", "Retransmission Rate"]
    data = dataframe.iloc[initial, measurements.index("Retransmission")]
    plt.table(cellText=[valueTypes, data], loc='center')
    plt.title(f"Retransmission Rate of {network}")
    plt.axis('off')
    # plt.savefig(f"{network}_Retransmission_Table", bbox_inches="tight")
    plt.show()

# The main function
if __name__ == '__main__':
    # Run dataCollector.sh separately
    # Initializing the dataframes for both Spectrum and CaseWireless
    spectrumData = pd.DataFrame(index=np.arange(totalData), columns=measurements) # Number of rows preset to 1000 & columns as measurements
    CWData = pd.DataFrame(index=np.arange(totalData), columns=measurements) # Same as above but for CaseWireless data
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
        plotRTT(network, df, rttInfo)
        plotPacketLoss(network, df)
        plotJitter(network, df)
        plotHops(network, df)
        plotThroughput(network, df)
        plotBandwidth(network, df)
        plotRetransmission(network, df)
    
    