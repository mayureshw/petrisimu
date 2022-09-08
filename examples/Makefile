CPPFLAGS	+=	-O3
LDFLAGS		+=	-lpthread
SRCS		=	$(wildcard *.cpp)
HDRS		=	$(wildcard ../*.h)
BINS		=	$(SRCS:.cpp=.out)

%.out: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

all: $(BINS)

clean:
	rm -f $(BINS)

include $(PETRISIMUDIR)/Makefile.petrisimu
