TARGETS=proxy
all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxy: proxyDaemon.cpp
	g++ -pedantic -Werror -Wall -pthread -lpthread -std=gnu++11 -o proxy proxyDaemon.cpp