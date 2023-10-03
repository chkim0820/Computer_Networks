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