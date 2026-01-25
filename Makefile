# Normal build
compile: build run

build:
	gcc ./src/*.c ./src/clay/*.c ./src/layout/*.c ./src/scheme/*.c ./src/subeditor/*.c -o ./out/app -lSDL3 -lSDL3_ttf -lchibi-scheme \
		&& cp ./resources/init.scm ./out/resources/init.scm \
		&& cp ./resources/JetBrainsMono.ttf ./out/resources/JetBrainsMono.ttf \
		&& cp ./resources/JetBrainsMono-Italic.ttf ./out/resources/JetBrainsMono-Italic.ttf \
		&& cp ./resources/VictorMono-Regular.ttf ./out/resources/VictorMono-Regular.ttf \
		&& cp ./resources/VictorMono-Bold.ttf ./out/resources/VictorMono-Bold.ttf \
		&& cp ./resources/VictorMono-Italic.ttf ./out/resources/VictorMono-Italic.ttf \

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
