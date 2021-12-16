AOCL_COMPILE_CONFIG=$(shell aocl compile-config)
AOCL_LINK_CONFIG=$(shell aocl link-config)

CC = g++
CCFLAGS = -std=c++11 -fPIC -g


DBSCAN2 : dbscan2.o
	$(CC) $(CCFLAGS) -o DBSCAN2 dbscan2.o

DBSCAN : main.o utility.o
	$(CC) $(CCFLAGS) $(AOCL_LINK_CONFIG) -lOpenCL -o DBSCAN main.o utility.o

main.o : main.cpp
	$(CC) $(CCFLAGS) $(AOCL_COMPILE_CONFIG) -o main.o -c main.cpp

utility.o : utility.cpp utility.h
	$(CC) $(CCFLAGS) $(AOCL_COMPILE_CONFIG) -o utility.o -c utility.cpp

dbscan2.o : DBSCAN2.cc
	$(CC) $(CCFLAGS) -o dbscan2.o -c DBSCAN2.cc

clean :
	rm -rf DBSCAN *.o
