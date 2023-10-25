# Name: Chaehyeon Kim
# Case ID: cxk445
# Filename: Makefile
# Date created: 10/25/2023
# Description: Makefile for executing proj4.cpp

CXX = g++
CFLAGS = -Wall -Werror -g
TARGETS = proj4

all: $(TARGETS)

proj2: proj4.cpp
	$(CXX) $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGETS)