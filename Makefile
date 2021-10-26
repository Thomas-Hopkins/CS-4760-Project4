CC = gcc
CFLAGS = -Wall -g

LIB = lib
LIBS = log

LIBLOG = liblog

EXE = oss osschild
DEPS = config.h
OBJS =

CLEAN = $(EXE) *.o $(OBJS) $(LIB)/$(LIBLOG).a $(LIB)/*.h logfile

all: $(LIB)/$(LIBLOG).a $(EXE)

%sim: %sim.o $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) -Llib -l$(LIBS)

$(LIB)/$(LIBLOG).a: $(LIB)/$(LIBLOG)/Makefile
	echo $(LIB)/$(LIBLOG).a make
	make -C lib/liblog
	cp lib/liblog/log.h lib
	mv lib/liblog/liblog.a lib

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(CLEAN)
	make -C lib/liblog clean
