#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "instructionFunctions.h"
#include "main.h"
#define MEM_SIZE 524288

// simulated memory and architecture
uint64_t registers[32];
unsigned char programMemory[524288]; // 512KiB
int64_t programCounter = 0x1000;

int main(int argc, char *argv[])
{
	registers[31] = MEM_SIZE;
	char *inputPath = argv[1];
	if (inputPath == NULL)
	{
		throwError("Invalid tinker filepath");
	}
	loadFile(inputPath);
	runSimulation();
	return 0;
}

void loadFile(char *inputPath)
{
	FILE *inputBinaryFile;
	if ((inputBinaryFile = fopen(inputPath, "rb")) != NULL)
	{
		uint64_t file_type = 0;
		uint64_t code_seg_begin = 0;
		uint64_t code_seg_size = 0;
		uint64_t data_seg_begin = 0;
		uint64_t data_seg_size = 0;

		fread(&file_type, sizeof(file_type), 1, inputBinaryFile);
		fread(&code_seg_begin, sizeof(file_type), 1, inputBinaryFile);
		fread(&code_seg_size, sizeof(file_type), 1, inputBinaryFile);
		fread(&data_seg_begin, sizeof(file_type), 1, inputBinaryFile);
		fread(&data_seg_size, sizeof(file_type), 1, inputBinaryFile);

		fread(&programMemory[code_seg_begin], 1, code_seg_size, inputBinaryFile);
		fread(&programMemory[data_seg_begin], 1, data_seg_size, inputBinaryFile);
		programCounter = code_seg_begin;
	}
	else
	{
		throwError("Invalid tinker filepath");
	}
	fclose(inputBinaryFile);
}

void throwError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

void runSimulation()
{

	setStatics(registers, programMemory, &programCounter);
	// defines a standard function header so I can make an array of them to auto exec when hit certain op codes
	typedef void (*genericInstructionFunction)(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L);
	// Apparently this is called a dispatch table - where you can set which func pointers at which index then run it
	// wait I now lowk realize that I could've just put them in order
	// but whatevs, this is cooler despite taking f'n forever :(
	genericInstructionFunction dispatchTable[30] = {
		[0x0] = and,
		[0x1] = or,
		[0x2] = xor,
		[0x3] = not,
		[0x4] = shftr,
		[0x5] = shftri,
		[0x6] = shftl,
		[0x7] = shftli,
		[0x8] = br,
		[0x9] = brr_reg,
		[0xa] = brr_lit,
		[0xb] = brnz,
		[0xc] = call,
		[0xd] = run_return,
		[0xe] = brgt,
		[0xf] = priv,
		[0x10] = mov_load,
		[0x11] = mov_reg,
		[0x12] = mov_lit,
		[0x13] = mov_store,
		[0x14] = addf,
		[0x15] = subf,
		[0x16] = mulf,
		[0x17] = divf,
		[0x18] = add,
		[0x19] = addi,
		[0x1a] = sub,
		[0x1b] = subi,
		[0x1c] = mul,
		[0x1d] = run_div,
	};

	while (1)
	{
		// should check to make sure that valid before we keep going
		if (programCounter + 4 > MEM_SIZE || programCounter < 0x1000)
		{
			printf("broken here\n programCounter: 0x%x \n", programCounter);
			printf("broken here\n memSize: 0x%x \n", MEM_SIZE);

			throwError("Simulation error");
		}
		uint32_t fullInstruction;
		memcpy(&fullInstruction, &programMemory[programCounter], sizeof(uint32_t));

		uint8_t opcode = (fullInstruction >> 27);
		uint8_t rd = (fullInstruction >> 22) & 0x1F;
		uint8_t rs = (fullInstruction >> 17) & 0x1F;
		uint8_t rt = (fullInstruction >> 12) & 0x1F;
		uint32_t L = fullInstruction & 0xFFF;
		if (opcode >= 30 || dispatchTable[opcode] == NULL)
		{
			printf("broken here2\n");
			throwError("Simulation error");
		}
		dispatchTable[opcode](rd, rs, rt, L);
	}
}