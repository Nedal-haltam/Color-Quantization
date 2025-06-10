

all: build

build: main.c
	gcc main.c -Wall -Wextra -Wpedantic -Iraylib/include -Lraylib/lib -lraylib -lgdi32 -lwinmm -o quantize.exe
