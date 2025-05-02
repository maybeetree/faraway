
build: treeindex.c
	@mkdir -p build
	gcc -g -ansi -pedantic -Wfatal-errors -Werror treeindex.c -o build/treeindex

debug: build
	gdb ./build/treeindex
