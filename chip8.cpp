#include "chip8.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

unsigned char chip8Fontset[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

void chip8::decode() {
	//11111111 << 8
	//1111111100000000 |
	//        11111111
	//1111111111111111 = 2 bytes = one opcode which is 2 bytes long
	opcode = (memory[pc] << 8) | memory[pc + 1];

	//1010001011110000 & 0xA2F0
	//0000111111111111 = 0x0FFF
	//0000001011110000   0x02F0 = 0x2F0
	switch(opcode & 0xF000) {
		case 0x0000:
				switch (opcode & 0x000F) {
					case 0x0000: // 00E0: clears the screen
						for (int i = 0; i < 2048; ++i)
							gfx[i] = 0x0;
						drawFlag = true;
						pc += 2;
						break;
					case 0x000E: // 00EE: Returns from subroutine
						--sp; //decrease since we increase sp from previous opcode
						pc = chip8Stack[sp]; // put stored return address back into program counter
						pc += 2; //increase pc so we do not call the same subroutine again
						break;
					default:
						std::cout << "Unkown opcode" << opcode << std::endl;
						break;
				}
			break;
		case 0x1000: // 1NNN: Jump to address NNN
			pc = opcode & 0x0FFF;
			break;
		case 0x2000: // 2NNN: Calls subroutine at address NNN
			chip8Stack[sp] = pc;
			++sp;
			pc = opcode & 0x0FFF;
			break;
		case 0x3000: // 3XRR: Skip next instruction if VX == RR
			if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
				pc += 4;
			else
				pc += 2;
			break;
		case 0x4000: // 4XRR: Skip next instruction if VX != RR
			if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
				pc += 4;
			else
				pc += 2;
			break;
		case 0x5000: // 5XY0: Skip next instruction if VX == VY
			if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
				pc += 4;
			else
				pc += 2;
			break;
		case 0x6000: // 6XRR: move constant RR(8bits) to VX
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
			break;
		case 0x7000: // 7XRR: Add RR to VX
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			pc += 2;
			break;
		case 0x8000: // 8XY?
			switch (opcode & 0x000F) {
				case 0x0000: // 8XY0: move VY into VX
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0001: // 8XY1: Bitwise OR VY with VX, store result in VX
					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0002: // 8XY2: Bitwise AND VY with VX, store in VX
					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0003: // 8XY3: XOR VY with VX, store in VX
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0004: // 8XY4: add VY to VX, store in VX, carry stored in VF
					if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
						V[0xF] = 1; //carry
					else
						V[0xF] = 0;
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0005: // 8XY5: subtract VY from VX, VF set to 0 if there is a borrow
					//VX = VX - VY
					if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
						V[0xF] = 0; // there is a borrow
					else
						V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;
					break;
				case 0x0006: // 8X06: shift VX right, bit 0 goes in VF
					V[0xF] = (V[(opcode & 0x0F00) >> 8]) & 0x1;
					V[(opcode & 0x0F00) >> 8] >>= 1;
					pc += 2;
					break;
				case 0x0007: // 8XY7: subtract VX from VY, VF set to 0 if there is a borrow
					//VX = VY - VX
					if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
						V[0xF] = 0;
					else
						V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;
				case 0x000E: // 8X0E: shift VX left, bit 7 stored in VF
					V[0xF] = (V[(opcode & 0x0F00) >> 8]) >> 7;
					V[(opcode & 0x0F00) >> 8] <<= 1;
					pc += 2;
					break;
				default:
					std::cout << "Unkown opcode" << opcode << std::endl;
					break;
			}
			break;
		case 0x9000: // 9XY0: Skips the next instruction if VX != VY
			if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
				pc += 4;
			else
				pc += 2;
			break;
		case 0xA000: // ANNN: Sets I to the address NNN
			I = opcode & 0x0FFF;
			pc += 2;
			break;
		case 0xB000: // BNNN: Jumps to the address NNN plus V0
			pc = (opcode & 0x0FFF) + V[0];
			break;
		case 0xC000: // CXNN: Sets VX to a random number (max 255) and
			//       Bitwise AND NN
			V[(opcode & 0x0F00) >> 8] = (rand() % 0x100) & (opcode & 0x00FF);
			pc += 2;
			break;
		case 0xD000: { // DXYN: https://en.wikipedia.org/wiki/CHIP-8#Opcode_table
			unsigned char x = V[(opcode & 0x0F00) >> 8];
			unsigned char y = V[(opcode & 0x00F0) >> 4];
			unsigned char height = opcode & 0x000F;
			unsigned char pixel;

			V[0xF] = 0;
			for (unsigned char yline = 0; yline < height; ++yline) {
				pixel = memory[I + yline];
				for (unsigned char xline = 0; xline < 8; ++xline) {
					if ((pixel & (0x80 >> xline)) != 0) {
						if (gfx[x + xline + ((y + yline) * 64)] == 1)
							V[0xF] = 1;
						gfx[x + xline + ((y + yline) * 64)] ^= 1;
						//We only care if the pixel value is 1 because we are
						//XOR the pixel of the sprite with the corresponding pixel
						//on the screen, if the pixel sprite value is 0, it does not
						//change the value of the pixel on the screen
						//We only XOR if the pixel sprite value is 1
					}
				}
			}
			drawFlag = true;
			pc += 2;
			break;
		}
		case 0xE000:
			switch (opcode & 0x000F) {
				case 0x000E: // EX9E: Skips next instruction if key stored in VX is pressed
					if (key[V[(opcode & 0x0F00) >> 8]] == 1)
						pc += 4;
					else
						pc += 2;
					break;
				case 0x0001: // EXA1: Skips next instruct if key stored in VX isn't pressed
					if (key[V[(opcode & 0x0F00) >> 8]] == 0)
						pc += 4;
					else
						pc += 2;
					break;
			}
			break;
		case 0xF000:
			switch (opcode & 0x00FF) {
				case 0x0007: // FX07: sets VX to value of delay timer
					V[(opcode & 0x0F00) >> 8] = delayTimer;
					pc += 2;
					break;
				case 0x000A: { // FX0A: A key press is awaited, then stored in VX
					bool keyPress = false;
					for (int i = 0; i < 16; ++i) {
						if (key[i] != 0) {
							V[(opcode & 0x0F00) >> 8] = i;
							keyPress = true;
						}
					}
					if (!keyPress)
						return;
					pc += 2;
					break;
				}
				case 0x0015: // FX15: Sets the delay timer to VX
					delayTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;
				case 0x0018: // FX18: Sets the sound timer to VX
					soundTimer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;
				case 0x001E: // FX1E: Adds VX to I
					if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
						V[0xF] = 1;
					else
						V[0xF] = 0;
					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
					break;
				case 0x0029: // FX29: Sets I to the location of the sprite for the
					//       character in VX
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
					pc += 2;
					break;
				case 0x0033: // FX33: Stores the BCD of VX
					memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					pc += 2;
					break;
				case 0x0055: // FX55: Stores V0 to VX in memory starting at adress I
					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
						memory[I + i] = V[i];
					// On original interpreter, when done, I = I + X + 1
					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
					break;
				case 0x0065: // FX65: Fills V0 to VX with vales from memory starting at Address I
					for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
						V[i] = memory[I + i];
					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
					break;

				default:
					std::cout << "Unkown opcode" << opcode << std::endl;
					break;
			}
			break;
		default:
			std::cout << "Unkown opcode" << opcode << std::endl;
			break;
	}

	if (delayTimer > 0)
    --delayTimer;
  if (soundTimer > 0)
    --soundTimer;
    //Add if to check when soundTimer == 1 and play sound
}

void chip8::init() {
	pc = 0x200;
	opcode = 0;
	I = 0;
	sp = 0;

	for (int i = 0; i < 2048; ++i)
		gfx[i] = 0;

	for (int i = 0; i < 16; ++i) {
		chip8Stack[i] = 0;
		key[i] = 0;
		V[i] = 0;
	}

	for (int i = 0; i < 4096; ++i)
		memory[i] = 0;

	for (int i = 0; i < 80; ++i)
		memory[i] = chip8Fontset[i];

	delayTimer = 0;
	soundTimer = 0;

	drawFlag = true;

	srand(time(NULL));
}

void chip8::consoleRenderer() {
// Draw Left to Right
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 64; ++x) {
			if (gfx[((y * 64) + x)] == 0)
				std::cout << "0";
			else
				std::cout << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

bool chip8::loadROM(const char *romName) {
	init();

	std::streampos romSize;
	char *romBuffer;
	std::ifstream file (romName, std::ios::in|std::ios::binary|std::ios::ate);
	if (file.is_open()) {
		romSize = file.tellg();
		romBuffer = new char [romSize];
		file.seekg(0, std::ios::beg);
		file.read(romBuffer, romSize);
		file.close();
	} else {
		std::cout << "Unable to open file";
		return false;
	}
	//4096-512(0x200) = 3584
	if(3584 > romSize) {
		for (int i = 0; i < romSize; ++i)
			memory[i + 512] = romBuffer[i];
	} else {
		std::cout << "ROM too big for memory";
		return false;
	}

	delete[] romBuffer;
	return true;
}
