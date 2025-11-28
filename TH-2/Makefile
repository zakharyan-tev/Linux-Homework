CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -fPIC
LDFLAGS = -pthread

LIB = lib_sch.so
DEMO = demo-application

all: $(LIB) $(DEMO)

$(LIB): lib_sch.o
	$(CXX) -shared -o $(LIB) lib_sch.o

lib_sch.o: lib_sch.cpp lib_sch.h
	$(CXX) $(CXXFLAGS) -c lib_sch.cpp

$(DEMO): main.o $(LIB)
	$(CXX) -o $(DEMO) main.o -L. -lsch -Wl,-rpath,. $(LDFLAGS)

main.o: main.cpp lib_sch.h
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f *.o $(LIB) $(DEMO)
