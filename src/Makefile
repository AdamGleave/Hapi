CPP = g++
DEBUG = -g
FLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 $(DEBUG)
CPPFLAGS = $(FLAGS) -c
LFLAGS = $(FLAGS)

clean:
	rm *.o

DIMACSEcho: DIMACSEcho.o DIMACS.o Arc.o FlowNetwork.o 

all: DIMACSEcho
