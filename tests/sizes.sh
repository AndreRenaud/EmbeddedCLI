#!/bin/bash

# Configuration
CC=arm-none-eabi-gcc
SIZE=arm-none-eabi-size
NM=arm-none-eabi-nm

# Use -ffreestanding and provide minimal headers to avoid toolchain dependencies
CFLAGS="-mthumb -mcpu=cortex-m3 -Os -I. -std=c99 -ffreestanding"

# Create a mock include directory
mkdir -p mock_incl

cat <<EOF > mock_incl/string.h
#include <stddef.h>
void *memset(void *s, int c, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
char *strstr(const char *haystack, const char *needle);
EOF

# Compile embedded_cli.c to object file
$CC $CFLAGS -Imock_incl -c embedded_cli.c -o embedded_cli.o

if [ $? -ne 0 ]; then
    echo "Error: Compilation of embedded_cli.c failed"
    rm -rf mock_incl
    exit 1
fi

echo "Binary sizes (ARM Thumb-2, -Os):"
$SIZE embedded_cli.o

# Calculate size of the structure
cat <<EOF > struct_size.c
#include "embedded_cli.h"
struct embedded_cli cli;
EOF

$CC $CFLAGS -Imock_incl -c struct_size.c -o struct_size.o
if [ $? -ne 0 ]; then
    echo "Error: Compilation of struct_size.c failed"
    rm -rf mock_incl embedded_cli.o struct_size.c
    exit 1
fi

# Get size using nm
STRUCT_SIZE_HEX=$($NM -S struct_size.o | grep " cli" | awk '{print $2}')

if [ -z "$STRUCT_SIZE_HEX" ]; then
    echo "Error: Could not find 'cli' symbol in object file"
else
    # Convert hex size from nm to decimal
    STRUCT_SIZE_DECIMAL=$((16#$STRUCT_SIZE_HEX))
    echo ""
    echo "Size of struct embedded_cli: $STRUCT_SIZE_DECIMAL bytes"
fi

# Cleanup
rm -rf embedded_cli.o struct_size.c struct_size.o mock_incl
