CPPFILES = $(shell ls *.cpp)
OBJFILES = $(CPPFILES:%.cpp=%.o)
OBJS = $(addprefix ../build/,$(OBJFILES))

all: ../build ../build/c3

../build:
	mkdir -p ../build

../build/%.o: %.cpp common.h c3.h
	g++ -c -O1 -ggdb -std=c++11 -o $@ $< 

../build/c3: $(OBJS)
	g++ $(OBJS) -o ../build/c3 -lcrypto

clean:
	rm -f ../build/*
