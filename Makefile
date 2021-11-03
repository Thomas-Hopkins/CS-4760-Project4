CC = gcc
CFLAGS = -Wall -g

LIB = lib
LIBS = log

LIBLOG = liblog

EXE = oss osschild
DEPS = shared.h
OBJS = shared.o

CLEAN = $(EXE) *.o $(OBJS) $(LIB)/$(LIBLOG).a $(LIB)/*.h logfile

all: $(LIB)/$(LIBLOG).a $(EXE)

oss: oss.o $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) -Llib -l$(LIBS)

osschild: osschild.o $(OBJS) $(DEPS)
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
