#
# Chip8 Emulator
# Written by Inseok Hwang
#

CC = gcc
CFLAGS = -std=c99 -Werror

default:	Chip8

Chip8:		chip8.o

chip8.o:	chip8.c
		$(CC) $(CFLAGS) chip8.c