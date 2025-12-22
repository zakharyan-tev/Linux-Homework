all: init writer reader

init: init.cpp
	g++ init.cpp -o init -pthread

writer: writer.cpp
	g++ writer.cpp -o writer -pthread

reader: reader.cpp
	g++ reader.cpp -o reader -pthread

clean:
	rm -f init writer reader
