CC = gcc
CFLAGS = -Wall -g

LIBLOG = liblog
LIBLOG_ARC = liblog.a
LIBLOG_SRC = log.c addmsg.c clearlog.c getlog.c savelog.c
LIBLOG_OBJ = log.o addmsg.o clearlog.o getlog.o savelog.o

CLEAN = $(LIBLOG_OBJ) $(LIBLOG_ARC)

all: $(LIBLOG_ARC)

$(LIBLOG_ARC): $(LIBLOG_OBJ)
	ar -rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(CLEAN)

