CC = g++
CPP = g++
DEBUG = -g
FLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 $(DEBUG)
CPPFLAGS = $(FLAGS)
LFLAGS = $(FLAGS)

all: DIMACSEcho RunCycleCancelling

DIMACSEcho: DIMACSEcho.o DIMACS.o Arc.o ResidualNetwork.o

RunCycleCancelling: RunCycleCancelling.o CycleCancelling.o EdmondsKarp.o BellmanFord.o Arc.o ResidualNetwork.o ResidualNetworkUtil.o DIMACS.o 

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

clean:
	rm *.o
