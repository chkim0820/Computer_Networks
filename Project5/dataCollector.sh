#!/bin/bash

# Name: Chaehyeon Kim (cxk445)
# Filename: dataColelctor.sh
# Date created: 12/03/2023
# Description: A script for collecting network data

echo "Running dataCollector.sh for collecting network data"
echo

# 1) ping the websites
echo "Pinging the websites"

echo "Pinging google.com"
ping google.com -c 1000 >> pingGoogle.txt

echo "Pinging case.edu"
ping case.edu -c 1000 >> pingCase.txt

echo "Pinging canvas.case.edu"
ping canvas.case.edu -c 1000 >> pingCanvas.txt

echo "Pinging instagram.com"
ping instagram.com -c 1000 >> pingInstagram.txt

echo "Pinging amazon.com"
ping amazon.com -c 1000 >> pingAmazon.txt

echo "Pinging youtube.com"
ping youtube.com -c 1000 >> pingYoutube.txt

echo "Pinging complete"
echo

# 2) Using iperf on the websites
echo "iPerf for bandwidth/retransmission"
iperf -c 172.20.2.106 -p 5001 -i 1 -t 1000 >> iperf.txt
echo

# 3) Using traceroute
echo "Tracerouting the websites"

for i in {1..1000}; do  # Change '10' to the desired number of iterations
    traceroute google.com >> trGoogle.txt
    traceroute case.edu >> trCase.txt
    traceroute canvas.case.edu >> trCanvas.txt
    traceroute instagram.com >> trInstagram.txt
    traceroute amazon.com >> trAmazon.txt
    traceroute youtube.com >> trYoutube.txt
done

echo "Tracerouting complete"
echo

# 4) Using netstat
echo "Using netstat"
netstat -s >> netstat.txt