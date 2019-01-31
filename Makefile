OPTFLAGS = -Wall -Wextra -O2 -ggdb -pipe
CFLAGS = $(OPTFLAGS)
CXXFLAGS = $(OPTFLAGS) -isystem libfmt/include -DFMT_HEADER_ONLY -std=c++17

mdt: main.o jackapi.h notify.h
	g++ $(CXXFLAGS) -o $@ $^ -ljack
all: mdt

clean:
	-rm -f *.o
	-rm -f mdt

run: mdt
	./mdt
