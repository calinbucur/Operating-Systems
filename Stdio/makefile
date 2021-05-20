default: build

first:
	gcc -fPIC -c libso_stdio.c

build: first
	gcc -shared libso_stdio.o -o libso_stdio.so

clean:
	rm -rf libso_stdio.o libso_stdio.so