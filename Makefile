compile: build run

build:
	gcc ./src/*.c ./src/clay/*.c ./src/layout/*.c ./src/scheme/*.c ./src/subeditor/*.c -o ./out/app -lSDL3 -lSDL3_ttf -lchibi-scheme

run:
	./out/app
