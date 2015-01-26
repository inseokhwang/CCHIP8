#
# Chip8 Emulator
# Written by Inseok Hwang
#

CC = gcc
CFLAGS = -std=c99 -Werror 
LINKLINE = -lglut -lGLU -lGL
EXECUTABLE = CCHIP-8

DRAWWITHREFRESH = -D DRAWWITHREFRESH

default:	chip8.c
		$(CC) $(CFLAGS) chip8.c $(LINKLINE)

refresh60:	chip8.c
		$(CC) $(CFLAGS) $(DRAWWITHREFRESH) chip8.c $(LINKLINE)

clean:		rm *.o