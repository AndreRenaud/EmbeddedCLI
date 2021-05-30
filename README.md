# Embedded CLI

Embedded CLI is a library for providing usable command line interfaces for embedded systems. It supports history and command line editing. It is designed to use no dynamic memory, and provide an easy to integrate API.

## Features
* Cursor support (left/right/up/down)
* Searchable history (^R to start search)
* No dynamic allocation
 * Base structure has a fixed size, with compile time size limitations
* Comprehensive test suite, including fuzz testing for memory safety

## Platform support
Embedded CLI makes very few assumptions about the platform. Data input/output is abstracted in call backs.

Examples are provided for a posix simulator, STM32

## Memory usage
Memory usage is configurable by adjusting the internal buffering. In the default configuration, it will consume 1.5kB of RAM, most of which is in the history buffer. The easiest way to reduce memory consumption is to reduce/drop the support for the history buffer.