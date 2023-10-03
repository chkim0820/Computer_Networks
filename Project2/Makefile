# Name: Chaehyeon Kim
# Case ID: cxk445
# Filename: Makefile
# Date created: 10/01/2023
# Description: Makefile for executing proj2.cpp

CXX = g++
CFLAGS = -Wall -Werror -g
TARGETS = proj2 test

all: $(TARGETS)

proj2: proj2.cpp
	$(CXX) $(CFLAGS) -o $@ $<

test: # Not sure if this works
	bash test

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $<

clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGETS)