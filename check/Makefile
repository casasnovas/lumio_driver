SOURCES=draw_mice.c
BIN=draw_mice
CLIBS=-lXi
#CFLAGS=-g -ggdb

all: draw_mice print_from_dev

draw_mice: draw_mice.c
	gcc draw_mice.c $(CFLAGS) $(CLIBS) -o draw_mice

print_from_dev: print_from_dev.c
	gcc print_from_dev.c -o print_from_dev

clean:
	rm -f draw_mice
	rm -f print_from_dev