CFLAGS=-g -Wall -Wextra -Werror -pipe -I. --std=c99
CLANG_FORMAT=clang-format
CLANG?=clang

SOURCES=embedded_cli.c embedded_cli.h tests/embedded_cli_test.c examples/posix_demo.c tests/embedded_cli_fuzzer.c

default: examples/posix_demo embedded_cli_test

test: embedded_cli_test
	./embedded_cli_test

fuzz: embedded_cli_fuzzer
	./embedded_cli_fuzzer -verbosity=0 -max_total_time=120 -max_len=8192 -rss_limit_mb=1024

examples/posix_demo: embedded_cli.o examples/posix_demo.o
	$(CC) -o $@ $^

embedded_cli_test: embedded_cli.o tests/embedded_cli_test.o
	$(CC) -o $@ $^

embedded_cli_fuzzer: embedded_cli.c tests/embedded_cli_fuzzer.c
	$(CLANG) -Itests -I. -g -O1 -o $@ tests/embedded_cli_fuzzer.c -fsanitize=fuzzer,address,undefined,integer

%.o: %.c
	# cppcheck --quiet --std=c99 --enable=warning,style,performance,portability,information  -I. -DTEST_FINI= $<
	$(CC) -c -o $@ $< $(CFLAGS)

format:
	$(CLANG_FORMAT) -i $(SOURCES)

clean:
	rm -f *.o */*.o embedded_cli_test embedded_cli_fuzzer examples/posix_demo
	rm -f timeout-* crash-*

.PHONY: clean format test default fuzz
