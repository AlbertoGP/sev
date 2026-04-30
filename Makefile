# Normal build
compile: build run

build:
	gcc -Wall -Wextra ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c -o ./out/sev -lSDL3 -lSDL3_image -lSDL3_ttf -lchibi-scheme -ltree-sitter -ltree-sitter-scheme -lm \
		&& cp ./scheme/init.scm ./out/scheme/init.scm \
		&& mkdir -p ./out/scheme/editor \
		&& mkdir -p ./out/scheme/editor/vim \
		&& mkdir -p ./out/scheme/editor/theme \
		&& cp ./scheme/editor/*.scm ./out/scheme/editor/ \
		&& cp ./scheme/editor/*.sld ./out/scheme/editor/ \
		&& cp ./scheme/editor/vim/*.scm ./out/scheme/editor/vim/ \
		&& cp ./scheme/editor/theme/*.scm ./out/scheme/editor/theme/

# Debug build with AddressSanitizer
debug: build-debug run

build-debug:
	gcc -g -O0 -fno-omit-frame-pointer -fsanitize=address -Wall -Wextra \
	    ./src/*.c ./src/clay/*.c ./src/command/*.c ./src/display/*.c ./src/text/*.c \
	    -o ./out/sev \
	    -lSDL3 -lSDL3_image -lSDL3_ttf -lchibi-scheme -ltree-sitter -ltree-sitter-scheme -lm \
	    && cp ./scheme/init.scm ./out/scheme/init.scm \
	    && mkdir -p ./out/scheme/editor \
	    && mkdir -p ./out/scheme/editor/vim \
	    && mkdir -p ./out/scheme/editor/theme \
	    && cp ./scheme/editor/*.scm ./out/scheme/editor/ \
	    && cp ./scheme/editor/*.sld ./out/scheme/editor/ \
	    && cp ./scheme/editor/vim/*.scm ./out/scheme/editor/vim/ \
	    && cp ./scheme/editor/theme/*.scm ./out/scheme/editor/theme/

vg:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --log-file="vglog" ./out/sev

run:
	./out/sev

clean:
	rm -f ./out/sev

# Unit tests
.PHONY: test test-text test-scheme
test: test-text test-scheme

test-text:
	mkdir -p ./out && \
	gcc -Wall -Wextra -I./test/unity -I./src \
	    ./test/unity/unity.c ./test/test_dead_keystroke.c ./test/test_stubs.c \
	    ./src/text/*.c \
	    -o ./out/sev_test \
	    -lchibi-scheme -ltree-sitter -ltree-sitter-scheme -lSDL3 -lm \
	    && ./out/sev_test

# Scheme-layer tests. Stages only the .scm/.sld files the chibi module
# loader needs into ./out/scheme — no dependency on the main editor build.
.PHONY: stage-scheme
stage-scheme:
	mkdir -p ./out/scheme/editor/vim ./out/scheme/editor/theme
	cp ./scheme/init.scm ./out/scheme/init.scm
	cp ./scheme/editor/*.scm ./out/scheme/editor/
	cp ./scheme/editor/*.sld ./out/scheme/editor/
	cp ./scheme/editor/vim/*.scm ./out/scheme/editor/vim/
	cp ./scheme/editor/theme/*.scm ./out/scheme/editor/theme/

test-scheme: stage-scheme
	mkdir -p ./out && \
	gcc -Wall -Wextra -I./test/unity -I./src \
	    ./test/unity/unity.c \
	    ./test/test_scheme_vim_motion.c \
	    ./test/scheme_test_init.c \
	    ./test/test_stubs.c \
	    ./src/text/*.c \
	    -o ./out/sev_scheme_test \
	    -lchibi-scheme -ltree-sitter -ltree-sitter-scheme -lSDL3 -lm \
	    && ./out/sev_scheme_test

# Website
website-sync-wasm:
	cp build-wasm/app.html build-wasm/app.js build-wasm/app.wasm build-wasm/app.data website/public/demo/

website-dev: website-sync-wasm
	cd website && bun run dev

website-build: website-sync-wasm
	cd website && bun run build
