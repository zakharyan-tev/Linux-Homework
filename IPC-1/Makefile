CXX = g++
CXXFLAGS = -std=c++17 -Wall

TARGET = signal-echo
SRC = signal_echo.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
