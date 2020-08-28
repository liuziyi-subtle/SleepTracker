# usage: make compiler=clang debug=1

CC = $(compiler) -g
CFLAGS = -Wall -Wextra

DFLAGS = -DALGO_DEBUG=$(debug)

DIR_DEBUG = debug

DIR_RESULTS = results

ETOBJS = main.o sleep_tracker.o sleep_depth.o sleep_utils.o sleep_model.o debug.o

all:	$(DIR_RESULTS) executable

$(DIR_RESULTS):
	mkdir $@

executable: $(ETOBJS)
	$(CC) $(CFLAGS) -o $@ $(ETOBJS) -lm

main.o:
	$(CC) $(CFLAGS) -c $(DIR_DEBUG)/main.c -o $@ -I$(DIR_DEBUG) $(DFLAGS)

sleep_tracker.o:
	$(CC) $(CFLAGS) -c sleep_tracker.c $(DFLAGS)

sleep_depth.o:
	$(CC) $(CFLAGS) -c sleep_depth.c $(DFLAGS)

sleep_utils.o:
	$(CC) $(CFLAGS) -c sleep_utils.c $(DFLAGS)

sleep_model.o:
	$(CC) $(CFLAGS) -c sleep_model.c $(DFLAGS)

debug.o:
	$(CC) $(CFLAGS) -c $(DIR_DEBUG)/debug.c -o $@ -I$(DIR_DEBUG) $(DFLAGS)

clean:
	rm -rf *.o *.out *~ executable $(DIR_RESULTS)