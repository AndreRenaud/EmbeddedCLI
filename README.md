# Embedded CLI

<img src="embedded_cli_logo.svg" alt="Embedded CLI Logo" width="200" align="right"/>

Embedded CLI is a library for providing usable command line interfaces for embedded systems. It supports history and command line editing. It is designed to use no dynamic memory, and provide an easy to integrate API.

[![GitHub Actions](https://github.com/AndreRenaud/EmbeddedCLI/workflows/Build%20and%20Test/badge.svg)](https://github.com/AndreRenaud/EmbeddedCLI/actions)

[![License: BSD 0-Clause](https://img.shields.io/badge/License-BSD%200--Clause-blue.svg)](LICENSE)

## Features
* Cursor support (left/right/up/down)
* Searchable history (^R to start search)
* No dynamic allocation
  * Base structure has a fixed size, with compile time size limitations
* Comprehensive test suite, including fuzz testing for memory safety
* Command line comprehension
  * Support for parsing the command line into an argc/argv pair
  * Handling of quoted strings, escaped characters etc...

Works well in conjunction with the [Simple Options](https://github.com/AndreRenaud/simple_options) library to provide quick & easy argument parsing in embedded environments. Using this combination makes it simple to create an extensible CLI interface, with easy argument parsing/usage/help support.

## Platform support & requirements
Embedded CLI makes very few assumptions about the platform. Data input/output is abstracted in call backs.

Examples are provided for a posix simulator, STM32

No 3rd party libraries are assumed beyond the following standard C library functions:
* memcpy
* memmove
* memset
* strcmp
* strlen
* strncpy
* strcpy

All code is C99 compliant.

## Memory usage
Memory usage is configurable by adjusting the internal buffering. In the default configuration, it will consume approximately 1.5kB of RAM, most of which is in the history buffer, and 2kB of code space (ARM Thumb2). The easiest way to reduce memory consumption is to drop the support for the history buffer. In this configuration it will consume approximately 200B of RAM, and approximately 1kB of code space (ARM Thumb2).

## Thanks
Icon `terminal` by Ashwin Dinesh from the Noun Project
