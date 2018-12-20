CFLAGS=$(shell pkg-config --cflags glib-2.0 libnm)
LDFLAGS=$(shell pkg-config --libs glib-2.0 libnm)
CC=gcc
PROGRAM=nm-wifi-wifi-handover
SRCS=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

format:
	@for file in $(SRCS) $(HEADERS); do \
		clang-format-6.0 -i "$$file"; \
	done

clean:
	rm $(PROJECT)

.PHONY: clean format
