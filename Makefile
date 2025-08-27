

all: build

build: main.c
	gcc main.c -o main -Iraylib/include -Lraylib/lib -l:libraylib.a -lm -g


