CX = g++
TARGET = read-file

all: $(TARGET)

$(TARGET): read-file.o
	$(CX) -o $(TARGET) read-file.o

read-file.o: read-file.cpp
	$(CX) -c read-file.cpp

clean: 
	rm -f $(TARGET) *.o
