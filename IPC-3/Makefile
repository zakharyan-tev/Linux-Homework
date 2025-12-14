CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -pthread

TARGETS = first second

all: $(TARGETS)

first: first.o shared_array.o
	$(CXX) $(CXXFLAGS) -o first first.o shared_array.o $(LDFLAGS)

second: second.o shared_array.o
	$(CXX) $(CXXFLAGS) -o second second.o shared_array.o $(LDFLAGS)

first.o: first.cpp shared_array.h
	$(CXX) $(CXXFLAGS) -c first.cpp

second.o: second.cpp shared_array.h
	$(CXX) $(CXXFLAGS) -c second.cpp

shared_array.o: shared_array.cpp shared_array.h
	$(CXX) $(CXXFLAGS) -c shared_array.cpp

clean:
	rm -f *.o first second
