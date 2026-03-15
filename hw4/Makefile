CFLAGS = -Wall -Wextra
LFLAGS = -lpthread -lm -mavx2

SRCS = $(shell find . -name '*.c')

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
	cc $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f $(PROGS)

