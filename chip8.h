#ifndef CHIP8_H_INCLUDED
#define CHIP8_H_INCLUDED

class chip8 {
	private:
		unsigned short int opcode;
		unsigned char memory[4096];
		unsigned char V[16];
		unsigned short int I;
		unsigned short int pc;
		unsigned char delayTimer;
		unsigned char soundTimer;
		unsigned short int chip8Stack[16];
		unsigned short int sp;
		void init();
	public:
		unsigned char gfx[64 * 32]; //2048 pixels
		unsigned char key[16];
		bool drawFlag;

		void decode();
		void consoleRenderer();
		bool loadROM(const char *romName);
	protected:

};


#endif // CHIP8_H_INCLUDED
