CXX = g++
CFLAGS = -Wall -Werror -g

TARGETS = proj2

all: $(TARGETS)

proj2: proj2.cpp
	$(CXX) $(CFLAGS) -o proj2 proj2.cpp

%.o: %.cc
	$(CXX) $(CFLAGS) -c $<

clean:
	rm -f *.o

distclean: clean
	rm -f $(TARGETS)