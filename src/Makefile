CC = g++
CPP = g++
DEBUG = -g
BASE_DIR=..
INCLUDE_DIR=$(BASE_DIR)/include
LIB_DIR=$(BASE_DIR)/lib
CPPFLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 -isystem $(INCLUDE_DIR) -O2
LDLIBS=-lboost_program_options -lgflags -lglog $(LIB_DIR)/libgtest.a

.PHONY:all
all: DIMACSEcho FindMinCost

DIMACSEcho: DIMACSEcho.o Arc.o ResidualNetwork.o

FindMinCost: FindMinCost.o CostScaling.o Arc.o FlowNetwork.o CycleCancelling.o \
			 EdmondsKarp.o BellmanFord.o ResidualNetwork.o ResidualNetworkUtil.o 

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o
	rm DIMACSEcho FindMinCost
