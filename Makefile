CXX = /opt/homebrew/Cellar/gcc/14.2.0_1/bin/g++-14
CXXFLAGS = -std=c++17 -Wall -pthread
LDFLAGS =

all: server client

server: server/server.cpp
	@echo "Building server..."
	$(CXX) $(CXXFLAGS) server/server.cpp -o server/server $(LDFLAGS)

client: client/client.cpp
	$(CXX) $(CXXFLAGS) client/client.cpp -o client/client $(LDFLAGS)

clean:
	rm -f server/server client/client