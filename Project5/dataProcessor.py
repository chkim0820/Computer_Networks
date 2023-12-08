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
# Total number of data points
totalData = 1000

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
        df["Round Trip Time"] = np.empty((len(spectrumData), 0)).tolist()
        df["Packet Loss"].iloc[0] = list() # Just need one for overall summary
        df["Jitter"] = np.empty((len(spectrumData), 0)).tolist()
        df["Number of Hops"] = np.empty((len(spectrumData), 0)).tolist()

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
        prevRTT = -1
        # Iterating through each lines except beginning & summary
        for i in range(1, totalLines - 4):
            line = lines[i] # Current line
            # RTT
            rttIndex = line.find("time=") + len("time=") # Index of where RTT value starts
            rttEnd = line.find("ms") # Index of where RTT value ends
            rtt = float(line[rttIndex: rttEnd])
            dataframe.iloc[i - 1, rttOrder].append(rtt) # Add RTT value to dataframe
            # Jitter
            jitter = abs(prevRTT - rtt) if (i != 1) else 0 # Jitter 0 if no prev. RTT
            dataframe.iloc[i - 1, jitterOrder].append(jitter) # Adding calculated jitter
            prevRTT = rtt
        # Fill in for missing data/packets (add None as placeholder)
        for i in range(totalData - (totalLines - 5)):
            dataframe.iloc[totalData - 1 - i, rttOrder].append(None)
            dataframe.iloc[totalData - 1 - i, jitterOrder].append(None)
        # Add the website's packet loss rate
        summaryLine = lines[totalLines - 2]
        recIndex = summaryLine.find("transmitted,") + len("transmitted,")
        recEnd = summaryLine.find("received")
        dataframe.iloc[0, lossOrder].append(summaryLine[recIndex: recEnd])
        # Save the additional RTT info provided at the end
        rttLine = lines[totalLines - 1]
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
    for i in range(6, len(lines) - 1):
        line = lines[i]
        # Throughput
        thIndex = line.find("sec") + len("sec")
        thEnd = line.find("GBytes") 
        throughput = float(line[thIndex: thEnd])
        dataframe.iloc[i - 6, throughputOrder] = throughput # Save for the nth iteration
        # Bandwidth
        bwIndex = line.find("GBytes") + len("GBytes")
        bwEnd = line.find("Gbits/sec") 
        bandwidth = float(line[bwIndex: bwEnd])
        dataframe.iloc[i - 6, bandwidthOrder] = bandwidth # Save for the nth iteration
        
# Process the traceroute data for each website
def processTraceRouteData(network, dataframe):
    maxHopOrder = measurements.index("Number of Hops")
    for website in websiteList:
        lines = openFile("tr", website, network) # traceroute file
        lineIt = 0 # Total number of lines iterated
        maxHop = 0 # Max number of hops for each traceroute command
        traceN = 0 # The nth iteration of separate traceroute commands
        # Traverse the data until 1000 entries are filled
        while (traceN < totalData):
            line = lines[lineIt]
            if (line.find("traceroute to") != -1): # If new traceroute command started
                if (lineIt != 0): # If not the first line
                    dataframe.iloc[traceN, maxHopOrder].append(maxHop)
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
    dataframe.iloc[0, measurements.index("Retransmission")] = [retransmissionRate, totalResend, totalSent]

# Calculating minimum, maximum, average, and standard deviation of measurement data
def calculateStats(data, numCols):
    stats = [] # For saving values to be returned
    for col in range(numCols):
        values = [] # To store all entries for the column/website
        totalSum = 0 # Total sum of all entries
        numRows = 0
        for row in range(len(data)):
            value = data.iloc[row][col] if (numCols==6) else data.iloc[row]
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
def plotPlots(network, data, measurement, unit, numCols=6, individual=False):
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
            if (numCols == 6): # Different values for each website
                y.append(data.iloc[i][website]) # measurement data
            elif (numCols == 1): # Values over all websites
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
def plotTable(network, data, measurement, unit, numCols=6, dataParsed=False):
    plt.clf() # Clearing the plot
    plt.title(f"{measurement} of {network} in {unit}")
    valueTypes = ["Average", "Minimum", "Maximum", "Standard Deviation"]
    if (dataParsed == False):
        data = calculateStats(data, numCols) # Input number of columns (websites vs. overall)
    if (numCols==6):
        plt.table(cellText=data, colLabels=valueTypes, rowLabels=websiteList, loc='center')
    elif (numCols==1):
        plt.table(cellText=[valueTypes, data[0]], loc='center')
    plt.axis('off')
    # plt.savefig(f"{network}_{measurement}_Table", bbox_inches="tight")
    plt.show()

# Plotting plots and graphs for RTT
def plotRTT(network, dataframe, rttInfo):
    plotPlots(network, dataframe["Round Trip Time"], "Round Trip Time", "ms")
    # Shifting around columns (min, avg, max) to (avg, min, max)
    for row in rttInfo:
        avg = row[1]
        row[1] = row[0]
        row[0] = avg
    # plotTable(network, rttInfo, "Round Trip Time", "ms", dataParsed=True)

# Creating a table for packets lost
def plotPacketLoss(network, dataframe):
    plt.clf() # Clearing the plot
    # Creating the list to create the table with
    packetsSent = [totalData] * len(websiteList)
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
    # plt.savefig(f"{network}_PacketLoss_Table", bbox_inches="tight")
    plt.show()

# Plotting plots and graphs for jitter
def plotJitter(network, dataframe):
    data = dataframe["Jitter"].iloc[1:len(dataframe)] # Exclude the first row
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
    plotPlots(network, data, "Throughput", "GBytes", numCols=1)
    # plotTable(network, data, "Throughput", "GBytes", numCols=1)

# Plotting plots and graphs for bandwidth
def plotBandwidth(network, dataframe):
    data = dataframe["Bandwidth"]
    plotPlots(network, data, "Bandwidth", "Gbits/sec", numCols=1)
    # plotTable(network, data, "Bandwidth", "Gbits/sec", numCols=1)

# Plotting plots and graphs for retransmission rate
def plotRetransmission(network, dataframe):
    plt.clf() # Clearing the plot
    valueTypes = ["Number Retransmitted", "Total Packets", "Retransmission Rate"]
    data = dataframe.iloc[0, 6]
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
        # plotPacketLoss(network, df)
        plotJitter(network, df)
        plotHops(network, df)
        plotThroughput(network, df)
        plotBandwidth(network, df)
        # plotRetransmission(network, df)
    