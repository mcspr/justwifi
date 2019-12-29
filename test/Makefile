.PHONY: all clean JustWifi.o

FIRMWARE_OBJ = Arduino.o WiFi.o IPAddress.o ESP.o
OBJECTS = jwtest.o JustWifi.o
CXXFLAGS := -g -I./ -I../src/
CC := g++

all: jwtest

clean:
	rm $(OBJECTS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) $< -std=c++17 -c -o $@

JustWifi.o: $(FIRMWARE_OBJ)
	$(CC) $(CXXFLAGS) -c ../src/JustWifi.cpp -o JustWifi.o

jwtest: $(FIRMWARE_OBJ) $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -lpthread -o $@

