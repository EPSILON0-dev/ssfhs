# Simple makefile for building SSFHS
FLAGS=-O0 -g -fsanitize=undefined -fsanitize=address -Wall -Wextra -Wpedantic

SRCS = src/args.c \
src/config.c \
src/socket.c \
src/http.c \
src/res.c \
src/utils.c \
src/main.c

OBJS = $(SRCS:src/%.c=build/%.o)

all: build/ssfhs

fresh: clean all

build_dir:
	mkdir -p build

build/ssfhs: $(OBJS) | build_dir
	gcc -o build/ssfhs $(FLAGS) $^

build/%.o: src/%.c | build_dir
	gcc -c $(FLAGS) -o $@ $<

.PHONY: clean
clean:
	-rm $(OBJS)
	-rm build/ssfhs
	-rmdir build
