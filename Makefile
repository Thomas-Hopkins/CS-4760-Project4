CC = gcc
CFLAGS = -Wall -g

EXE = oss osschild
DEPS = shared.h queue.h
OBJS = shared.o queue.o

CLEAN = $(EXE) *.o $(OBJS) logfile

all: $(EXE)

oss: oss.o $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS)

osschild: osschild.o $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) 

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(CLEAN)
