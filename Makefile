CXX=g++
CXX_FLAGS=-Wall -O2
CFLAGS=-Wall -O2 -c -fPIC

all: test

test: main

main: main.o pool.o
	$(CXX) $(CXX_FLAGS) main.o pool.o -o main

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ $<

clean:
	-rm -f main main.o pool.o