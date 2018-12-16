CFLAGS=$(shell pkg-config --cflags glib-2.0 libnm)
LDFLAGS=$(shell pkg-config --libs glib-2.0 libnm)
CC=gcc
PROGRAM=nm-wifi-wifi-handover
OBJS=main.o

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

clean:
	rm $(PROJECT)

