# GCC version: gcc 4.4.7
ROOTDIR = $(CURDIR)
CC = clang -g # gcc clang 
CXX = g++ -g
CFLAGS = -O3 -Wall -Wextra -Wl,-rpath=$(ROOTDIR) -std=c99
#LIB_DIR = -L$(ROOTDIR)
#LIB_DEP = -lModel
#XGBOOST_INCLUDE_DIR = -I$(ROOTDIR)/xgboost/include -I$(ROOTDIR)/xgboost/rabit/include
ETOBJS = main.o ls_sleep_tracker.o ls_sleep_predict.o ls_sleep_func.o

all:	main

main:	$(ETOBJS)
	$(CC) -o main $(ETOBJS) -Wall -lm -DGLOBAL_SLEEP_ALGO_OPEN=1 -DDEBUG_LOCAL=1

main.o: main.c ls_sleep_tracker.h
	$(CC) -c main.c -DGLOBAL_SLEEP_ALGO_OPEN=1 -DDEBUG_LOCAL=1

ls_sleep_tracker.o:	ls_sleep_tracker.c ls_sleep_tracker.h
	$(CC) -c ls_sleep_tracker.c -DGLOBAL_SLEEP_ALGO_OPEN=1 -DDEBUG_LOCAL=1

ls_sleep_predict.o:	ls_sleep_predict.c ls_sleep_predict.h
	$(CC) -c ls_sleep_predict.c -DGLOBAL_SLEEP_ALGO_OPEN=1 -DDEBUG_LOCAL=1

ls_sleep_func.o: ls_sleep_func.c ls_sleep_func.h
	$(CC) -c -Wall ls_sleep_func.c -DGLOBAL_SLEEP_ALGO_OPEN=1 -DDEBUG_LOCAL=1

clean:
	rm -f *.o *.out *~ main