# Normal build
compile: build run

build:
	gcc ./src/*.c ./src/clay/*.c ./src/layout/*.c ./src/scheme/*.c ./src/subeditor/*.c -o ./out/app -lSDL3 -lSDL3_ttf -lchibi-scheme

# Debug build with AddressSanitizer
debug: build-debug run

build-debug:
	gcc -fsanitize=address -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
	    ./src/*.c ./src/clay/*.c ./src/layout/*.c ./src/scheme/*.c ./src/subeditor/*.c \
	    -o ./out/app \
	    -fsanitize=address -lSDL3 -lSDL3_ttf -lchibi-scheme

vg:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --log-file="vglog" ./out/app

run:
	./out/app

clean:
	rm -f ./out/app
