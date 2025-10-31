# Simple makefile for building SSFHS
CC ?= cc

BUILD_MODE=DEBUG
# BUILD_MODE=RELEASE

ifeq ($(BUILD_MODE),RELEASE)
	FLAGS=-O2 -Wall -Wextra -Wpedantic
else
	FLAGS=-O0 -g -fsanitize=undefined -fsanitize=address -Wall -Wextra -Wpedantic
endif

SRCS = src/args.c \
src/config.c \
src/socket.c \
src/http.c \
src/res.c \
src/utils.c \
src/log.c \
src/main.c

OBJS = $(SRCS:src/%.c=build/%.o)

all: build/ssfhs

fresh: clean all

build_dir:
	mkdir -p build

build/ssfhs: $(OBJS) | build_dir
	$(CC) -o build/ssfhs $(FLAGS) $^

build/%.o: src/%.c | build_dir
	$(CC) -c $(FLAGS) -o $@ $<

.PHONY: clean
clean:
	-rm $(OBJS)
	-rm build/ssfhs
	-rmdir build
