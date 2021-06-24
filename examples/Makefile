CPPFLAGS=-I .. -O3
LDFLAGS=-lpthread
SRCS=$(wildcard *.cpp)
HDRS=$(wildcard ../*.h)
BINS=$(SRCS:.cpp=)

%: %.cpp $(HDRS)
	c++ $(CPPFLAGS) $< $(LDFLAGS) -o $@

all: $(BINS)

clean:
	rm -f $(BINS)
