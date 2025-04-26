CXX ?= g++
CFLAGS ?= -Wall -O2
LDFLAGS ?=

all: mydaemon

mydaemon: main.cpp
	$(CXX) $(CFLAGS) -o demon main.cpp $(LDFLAGS)

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 demon $(DESTDIR)/usr/bin/

clean:
	rm -f demon