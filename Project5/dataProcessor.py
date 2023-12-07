# Name/Case ID: Chaehyeon Kim (cxk445)
# File name: dataProcessor.py
# Description: For CSDS 325 Project 5; gathering measurements for different networks
# Date: 11/25/2023

import os

# A list of websites the network traffics lead to
websiteList = ["Amazon", "Canvas", "Case", "Google", "Instagram", "Youtube"]

# A function to open the specified file containing data
def openFile(fileType, destName="", network=""):
    folderDir = os.path.dirname(__file__) + "/" + network # Directory to the current script
    fileName = fileType + destName + ("CW" if network=="CaseWireless" else "") + ".txt"
    absFileDir = os.path.join(folderDir, fileName) # Absolute path of the file
    file = open(absFileDir, "r") # Contains the output
    return file

# Process the ping data files for all 6 sites, each for Spectrum and CaseWireless
def processPingData(network):
    for website in websiteList:
        openFile("ping", website, network)

def processIPerfData():
    print("iperf")

def processTraceRouteData():
    print("route data")

def processNetstatData():
    print("netstat")



if __name__ == '__main__':
    for network in ["Spectrum", "CaseWireless"]:
        processPingData(network)
    processIPerfData()
    processTraceRouteData()
    processNetstatData()

