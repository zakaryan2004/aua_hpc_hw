CFLAGS = -Wall -Wextra
LFLAGS = -lpthread -lm

SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
	cc $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f $(PROGS)

