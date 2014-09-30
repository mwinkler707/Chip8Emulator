#include <iostream>
#include "chip8.h"

chip8 chip8Emu;

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage: chip8.exe chip8ROM";
		return 1;
	}

	if (!chip8Emu.loadROM(argv[1]))
		return 1;

	while (true) {
		chip8Emu.decode();

		if (chip8Emu.drawFlag) {
			chip8Emu.consoleRenderer();
			chip8Emu.drawFlag = false;
		}
	}
	return 0;
}
