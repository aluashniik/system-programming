CC = gcc
CFLAGS = -Wall -g
SRCS = main.c list.c hash.c bitmap.c hex_dump.c debug.c
OBJS = $(SRCS:.c=.o)
TARGET = testlib

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) 

clean:
	rm -f $(OBJS) $(TARGET)  
