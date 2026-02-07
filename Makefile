# Normal build
compile: build run

build:
	gcc -Wall -Wextra ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c -o ./out/sev -lSDL3 -lSDL3_image -lSDL3_ttf -lchibi-scheme \
		&& cp ./scheme/init.scm ./out/scheme/init.scm \
		&& cp ./scheme/command.scm ./out/scheme/command.scm \
		&& cp ./scheme/mode.scm ./out/scheme/mode.scm \
		&& cp ./scheme/evil.scm ./out/scheme/evil.scm \
		&& cp ./scheme/theme.scm ./out/scheme/theme.scm

# Debug build with AddressSanitizer
debug: build-debug run

build-debug:
	gcc -fsanitize=address -g -O0 -fno-omit-frame-pointer -Wall -Wextra \
	    ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c \
	    -o ./out/sev \
	    -fsanitize=address -lSDL3 -lSDL3_ttf -lchibi-scheme

vg:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --log-file="vglog" ./out/sev

run:
	./out/sev

clean:
	rm -f ./out/sev
