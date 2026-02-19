#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "main.h"

#define startingAddress 0x1000
static uint64_t *staticRegisters;
static unsigned char *staticProgramMemory;
static int64_t *staticProgramCounter;

void setStatics(uint64_t *registers2, unsigned char *programMemory2, uint64_t *programCounter2)
{
	staticRegisters = registers2;
	staticProgramMemory = programMemory2;
	staticProgramCounter = programCounter2;
}

// instruction handlers actually implemented
void and(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] & staticRegisters[rt];
	*staticProgramCounter += 4;
}
void or(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] | staticRegisters[rt];
	*staticProgramCounter += 4;
}
void xor(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] ^ staticRegisters[rt];
	*staticProgramCounter += 4;
}
void not(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = ~staticRegisters[rs];
	*staticProgramCounter += 4;
}
void shftr(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] >> staticRegisters[rt];
	*staticProgramCounter += 4;
}
void shftri(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rd] >> L;
	*staticProgramCounter += 4;
}
void shftl(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] << staticRegisters[rt];
	*staticProgramCounter += 4;
}
void shftli(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rd] << L;
	*staticProgramCounter += 4;
}
void br(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	*staticProgramCounter = staticRegisters[rd];
}
void brr_reg(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	*staticProgramCounter += staticRegisters[rd];
}
void brr_lit(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	// basically since L is supposed to be signed, we convert it over if necessary
	if (L >> 11)
	{
		L |= 0xFFFFF000;
	}
	int32_t L2 = (int32_t)L;
	*staticProgramCounter += L2;
}
void brnz(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	if (staticRegisters[rs] == 0)
	{
		*staticProgramCounter += 4;
	}
	else
	{
		*staticProgramCounter = staticRegisters[rd];
	}
}
void call(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	int64_t address = (int64_t)staticRegisters[31] - 8;
	if (address + 8 > MEM_SIZE || address < 0) // since we're gonna copy 8 bytes into it
	{
		throwError("Simulation error");
	}
	uint64_t returnAddress = *staticProgramCounter + 4;
	memcpy(&staticProgramMemory[address], &returnAddress, sizeof(uint64_t));
	*staticProgramCounter = staticRegisters[rd];
}
void run_return(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	int64_t address = (int64_t)staticRegisters[31] - 8;
	if (address + 8 > MEM_SIZE || address < 0 ) // since we're gonna copy 8 bytes into it
	{
		throwError("Simulation error");
	}
	memcpy(staticProgramCounter, &(staticProgramMemory[address]), sizeof(uint64_t));
}
void brgt(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	if ((int64_t)staticRegisters[rs] <= (int64_t)staticRegisters[rt])
	{
		*staticProgramCounter += 4;
	}
	else
	{
		*staticProgramCounter = staticRegisters[rd];
	}
}
void priv(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	if (L == 0x0)
	{
		exit(EXIT_SUCCESS);
	}
	else if (L == 0x3)
	{
		if (staticRegisters[rs] == 0)
		{
			char line[256];
			if (fgets(line, sizeof(line), stdin) == NULL)
			{
				throwError("Simulation error");
			}
			char *start = line;
			errno = 0;
			char *endptr;
			int64_t value = strtoll(start, &endptr, 10);

			if (endptr == start || errno == ERANGE || value < 0 || (*endptr != '\0' && *endptr != '\n'))
			{
				throwError("Simulation error");
			}
			staticRegisters[rd] = (uint64_t) value;
		}
		*staticProgramCounter += 4;
	}
	else if (L == 0x4)
	{
		*staticProgramCounter += 4;
		if (staticRegisters[rd] == 1)
		{ // the only valid output port for this prog
			printf("%llu\n", staticRegisters[rs]);
		}
	}
	else
	{
		throwError("Simulation error");
	}
}
void mov_load(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	// basically since L is supposed to be signed, we convert it over if necessary
	if (L >> 11)
	{
		L |= 0xFFFFF000;
	}
	
	int64_t address = (int32_t)staticRegisters[rs] + (int32_t)L;

	if (address + 8 > MEM_SIZE || address < 0)
		throwError("Simulation error");
	memcpy(&(staticRegisters[rd]), &(staticProgramMemory[address]), sizeof(uint64_t));
	*staticProgramCounter += 4;
}
void mov_reg(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs];
	*staticProgramCounter += 4;
}
void mov_lit(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	// zero out the last 12 bits of stuff (assuming stored on end -- todo: check assumption -- then adding to masked of L where only has last 12 bits)
	staticRegisters[rd] = (staticRegisters[rd] & 0xFFFFFFFFFFFFF000ULL) | (L & 0xFFF);
	*staticProgramCounter += 4;
}
void mov_store(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	// basically since L is supposed to be signed, we convert it over if necessary
	if (L >> 11)
	{
		L |= 0xFFFFF000;
	}
	int64_t address = (int64_t) staticRegisters[rd] + (int32_t)L;
	if (address + 8 > MEM_SIZE || address < 0 )
		throwError("Simulation error");
	memcpy(&(staticProgramMemory[address]), &(staticRegisters[rs]), sizeof(uint64_t));
	*staticProgramCounter += 4;
}
void addf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	double a, b, result;
	memcpy(&a, &staticRegisters[rs], sizeof(double));
	memcpy(&b, &staticRegisters[rt], sizeof(double));
	result = a + b;
	memcpy(&staticRegisters[rd], &result, sizeof(double));
	*staticProgramCounter += 4;
}
void subf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	double a, b, result;
	memcpy(&a, &staticRegisters[rs], sizeof(double));
	memcpy(&b, &staticRegisters[rt], sizeof(double));
	result = a - b;
	memcpy(&staticRegisters[rd], &result, sizeof(double));
	*staticProgramCounter += 4;
}
void mulf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	double a, b, result;
	memcpy(&a, &staticRegisters[rs], sizeof(double));
	memcpy(&b, &staticRegisters[rt], sizeof(double));
	result = a * b;
	memcpy(&staticRegisters[rd], &result, sizeof(double));
	*staticProgramCounter += 4;
}
void divf(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	double a, b, result;
	memcpy(&a, &staticRegisters[rs], sizeof(double));
	memcpy(&b, &staticRegisters[rt], sizeof(double));
	if (b == 0)
	{
		throwError("Simulation error");
	}
	result = a / b;
	memcpy(&staticRegisters[rd], &result, sizeof(double));
	*staticProgramCounter += 4;
}
void add(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] + staticRegisters[rt];
	*staticProgramCounter += 4;
}
void addi(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] += L;
	*staticProgramCounter += 4;
}
void sub(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] - staticRegisters[rt];
	*staticProgramCounter += 4;
}
void subi(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] -= L;
	*staticProgramCounter += 4;
}
void mul(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	staticRegisters[rd] = staticRegisters[rs] * staticRegisters[rt];
	*staticProgramCounter += 4;
}
void run_div(uint8_t rd, uint8_t rs, uint8_t rt, uint32_t L)
{
	if (staticRegisters[rt] == 0 || (int64_t)staticRegisters[rt] == 0)
	{
		throwError("Simulation error");
	}
	staticRegisters[rd] = (uint64_t)((int64_t)staticRegisters[rs] / (int64_t)staticRegisters[rt]);
	*staticProgramCounter += 4;
}