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
    return file

# Set each entry of the specified columns to empty lists
def setEmptyLists(dataframes):
    for df in dataframes:
        df["RTT"] = np.empty((len(spectrumData), 0)).tolist()
        df["Packet Loss"].iloc[0] = list() # Just need one for overall summary
        df["Jitter"] = np.empty((len(spectrumData), 0)).tolist()
        df["Number of Hops"] = np.empty((len(spectrumData), 0)).tolist()


# Process the ping data files for all 6 sites, each for Spectrum and CaseWireless
# Lists created for each measurement types to contain all site information
def processPingData(network, dataframe):
    # Go through files for all websites
    for website in websiteList:
        file = openFile("ping", website, network) # Open the appropriate file
        lines = file.readlines() # Read all lines into lists
        totalLines = len(lines)
        prevRTT = -1
        # Iterating through each lines except beginning & summary
        for i in range(1, totalLines-4):
            line = lines[i] # Current line
            # RTT
            rttIndex = line.find("time=") + 5 # Index of where RTT value starts
            rttEnd = line.find(" ms") # Index of where RTT value ends
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
        recIndex = summaryLine.find("transmitted") + len("transmitted, ")
        recEnd = summaryLine.find(" received")
        dataframe.iloc[0, 1].append(summaryLine[recIndex: recEnd])


# Process the iperf data for each network 
def processIPerfData(network, dataframe):
    file = openFile("iperf", network=network)


def processTraceRouteData(network, dataframe):
    for website in websiteList:
        file = openFile("tr", website, network)

def processNetstatData(network, dataframe):
    file = openFile("netstat", network=network)



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

