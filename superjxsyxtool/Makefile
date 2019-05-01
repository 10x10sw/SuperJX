
BINDIR=$(PREFIX)/usr/bin

SOURCES=main.cpp patch.cpp tone.cpp
HEADERS=patch.h sysex.h tone.h

all: superjxsyxtool

superjxsyxtool: $(SOURCES) $(HEADERS)
	gcc $(SOURCES) -o superjxsyxtool -lstdc++ -std=c++11

clean:
	rm -f *.o superjxsyxtool

install:
	install -m 755 -o root -g root superjxsyxtool $(BINDIR)/
