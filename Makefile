# Normal build
compile: build run

build:
	gcc -Wall -Wextra ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c -o ./out/sev -lSDL3 -lSDL3_image -lSDL3_ttf -lchibi-scheme -ltree-sitter -ltree-sitter-scheme -lm \
		&& cp ./scheme/init.scm ./out/scheme/init.scm \
		&& mkdir -p ./out/scheme/editor \
		&& cp ./scheme/editor/*.scm ./out/scheme/editor/ \
		&& cp ./scheme/editor/*.sld ./out/scheme/editor/

# Debug build with AddressSanitizer
debug: build-debug run

build-debug:
	gcc -fsanitize=address -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
	    ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c \
	    -o ./out/sev \
	    -fsanitize=address -lSDL3 -lSDL3_ttf -lchibi-scheme -ltree-sitter -ltree-sitter-scheme

vg:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --log-file="vglog" ./out/sev

run:
	./out/sev

clean:
	rm -f ./out/sev
