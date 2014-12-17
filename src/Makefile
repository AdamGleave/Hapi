CC = g++
CPP = g++
DEBUG = -g
FLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 $(DEBUG)
CPPFLAGS = $(FLAGS)
LFLAGS = $(FLAGS)

clean:
	rm *.o

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

DIMACSEcho: DIMACSEcho.o DIMACS.o Arc.o ResidualNetwork.o 

all: DIMACSEcho EdmondsKarp.o
