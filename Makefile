PROGRAM = socket
ARGUMENTS = 4201

CC = gcc
CFLAGS = -pedantic -Wall -Wextra -std=gnu99 -ggdb3

LD = gcc	
LDFLAGS = -lm

CFILES = $(wildcard *.c)
OFILES = $(CFILES:.c=.o)

%.o: %.c #modern pattern rule
	$(CC) $(CFLAGS) -c $<

all: depend $(PROGRAM)

$(PROGRAM): $(OFILES)
	$(LD) -o $@ $(OFILES) $(LDFLAGS)

.PHONY: clean run debug

clean:
	rm -f $(OFILES) $(PROGRAM)

run: $(PROGRAM)
	./$(PROGRAM) $(ARGUMENTS)

debug:
	./gdb $(PROGRAM) $(ARGUMENTS)

DEPENDFILE = .depend

depend:
	$(CC) $(CFLAGS) -MM $(CFILES) > $(DEPENDFILE)

-include $(DEPENDFILE)
