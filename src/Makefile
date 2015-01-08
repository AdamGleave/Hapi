CC = g++
CPP = g++
DEBUG = -g
BASE_DIR=..
INCLUDE_DIR=$(BASE_DIR)/include
LIB_DIR=$(BASE_DIR)/lib
CPPFLAGS = -Wall -Wextra -ansi -pedantic -std=c++11 -isystem $(INCLUDE_DIR) -O2
LDLIBS=-lboost_program_options -lgflags -lglog $(LIB_DIR)/libgtest.a

.PHONY:all
all: dimacs_echo find_min_cost

dimacs_echo: dimacs_echo.o arc.o residual_network.o

find_min_cost: find_min_cost.o cost_scaling.o arc.o flow_network.o \
			   cycle_cancelling.o edmonds_karp.o bellman_ford.o \ 
			   residual_network.o residual_network_util.o

%.o : %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o
	rm dimacs_echo find_min_cost
