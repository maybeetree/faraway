
build: faraway.c
	@mkdir -p build
	gcc -g -ansi -pedantic -Wfatal-errors -Werror faraway.c -o build/faraway

buildstatic: faraway.c
	@mkdir -p build
	gcc -static -static-libgcc -g -ansi -pedantic -Wfatal-errors -Werror faraway.c -o build/faraway.static

debug: build
	gdb ./build/faraway

.PHONY: build buildstatic

