CFLAGS=-g -Wall -pipe -I.

default: examples/posix_demo embedded_cli_test

test: embedded_cli_test
	./embedded_cli_test

examples/posix_demo: embedded_cli.o examples/posix_demo.o
	$(CC) -o $@ $^

embedded_cli_test: embedded_cli.o embedded_cli_test.o
	$(CC) -o $@ $^

%.o: %.c
	cppcheck --quiet --std=c99 --enable=all -I. $<
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o */*.o