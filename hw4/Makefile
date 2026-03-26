CFLAGS = -Wall -Wextra
LFLAGS = -lpthread -lm -mavx2
INC_PATHS = -I../hpc_hw_lib/include

SRCS = $(shell find . -name '*.c')
CFLAGS += $(INC_PATHS)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
	cc $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f $(PROGS)

