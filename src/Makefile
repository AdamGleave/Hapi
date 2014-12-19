CC = g++
CPP = g++
DEBUG = -g
BASE_DIR=..
INCLUDE_DIR=$(BASE_DIR)/include
LIB_DIR=$(BASE_DIR)/lib
CPPFLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 -isystem $(INCLUDE_DIR) $(DEBUG)
LDLIBS=-lgflags -lglog $(LIB_DIR)/libgtest.a

all: DIMACSEcho RunCycleCancelling

DIMACSEcho: DIMACSEcho.o DIMACS.o Arc.o ResidualNetwork.o

RunCycleCancelling: RunCycleCancelling.o CycleCancelling.o EdmondsKarp.o BellmanFord.o Arc.o ResidualNetwork.o ResidualNetworkUtil.o DIMACS.o 

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o
