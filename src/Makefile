CC = g++
CPP = g++
DEBUG = -g
FLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 $(DEBUG)
CPPFLAGS = $(FLAGS)
LFLAGS = $(FLAGS)

all: DIMACSEcho EdmondsKarp.o BellmanFord.o CycleCancelling.o

DIMACSEcho: DIMACSEcho.o DIMACS.o Arc.o ResidualNetwork.o ResidualNetworkUtil.o 

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

clean:
	rm *.o

