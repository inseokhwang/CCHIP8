#include <stdio.h>

chip8 cpu;

int main(int argc, char **argv) {
	initGraphics();
	initInput();

	cpu.initialize();
	cpu.loadGame("pong");

	while(true) {
		chip.runCycle();
		if(chip.drawFlag) {
			drawScreen();
		}
		chip.setInput();
	}
	return 0;
}