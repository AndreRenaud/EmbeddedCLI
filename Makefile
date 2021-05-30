CFLAGS=-g -Wall -pipe -I.
CLANG_FORMAT=clang-format

SOURCES=embedded_cli.c embedded_cli.h embedded_cli_test.c examples/posix_demo.c

default: examples/posix_demo embedded_cli_test

test: embedded_cli_test
	./embedded_cli_test

examples/posix_demo: embedded_cli.o examples/posix_demo.o
	$(CC) -o $@ $^

embedded_cli_test: embedded_cli.o embedded_cli_test.o
	$(CC) -o $@ $^

%.o: %.c
	#cppcheck --quiet --std=c99 --enable=all -I. $<
	$(CC) -c -o $@ $< $(CFLAGS)

format:
	$(CLANG_FORMAT) -i $(SOURCES)

clean:
	rm -f *.o */*.o

.PHONY: clean format test default