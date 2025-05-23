

ARGS= .\Ada_lovelace.png .\output.png
# ARGS= .\Lena_512.png .\output.png

all: build run

build: main.c
	gcc main.c -Wall -Wextra -Wpedantic -Iraylib/include -Lraylib/lib -lraylib -lgdi32 -lwinmm -o quantize.exe
	
run: quantize.exe
	.\quantize.exe $(ARGS)

clean:
	del /Q quantize.exe 2>nul || exit 0