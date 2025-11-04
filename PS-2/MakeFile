TARGET = shell
CXX = g++
SRC = "Interactive Shell.cpp"
OBJ = InteractiveShell.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX)  -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CXX)  -c $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)
