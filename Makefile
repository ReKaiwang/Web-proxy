TARGETS=proxy
all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxy: proxyDaemon.cpp proxyDaemon.h
	g++ -pedantic -Werror -Wall -lpthread -std=gnu++98 -o $@ $<