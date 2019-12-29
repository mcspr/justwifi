.PHONY: all clean JustWifi.o

OBJECTS = jwtest.o Arduino.o WiFi.o IPAddress.o JustWifi.o ESP.o
CXXFLAGS := -g -I./ -I../src/
CC := g++

all: jwtest

clean:
	rm $(OBJECTS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) $< -std=c++17 -c -o $@

JustWifi.o:
	$(CC) $(CXXFLAGS) -c ../src/JustWifi.cpp -o JustWifi.o

jwtest: $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -lpthread -o $@

