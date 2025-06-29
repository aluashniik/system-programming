CC = gcc
CFLAGS = -Og
LDLIBS = -lpthread

PROGS = myShell

all: $(PROGS)

myShell: myshell.c myshell.h
	$(CC) $(CFLAGS) -o myShell myshell.c $(LDLIBS)

clean:
	rm -rf *~ $(PROGS)
