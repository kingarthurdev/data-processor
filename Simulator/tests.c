#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "instructionFunctions.h"

#define startingAddress 0x1000
#define MEM_SIZE 524288

// simulated memory and architecture
uint64_t registers[32];
unsigned char programMemory[524288]; // 512KiB
int64_t programCounter = 0x1000;

static int testsRun = 0;
static int testsPassed = 0;

// forward declarations
void testFileLoaded(int position, char *inputPath);
void modifiedLoadFile(char *inputPath);

void throwError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

// reset everything back to the starter before the test runs so we don't have to track it
void resetState()
{
	memset(registers, 0, sizeof(registers));
	memset(programMemory, 0, sizeof(programMemory));
	programCounter = startingAddress;
	registers[31] = MEM_SIZE; // stack pointer init
	setStatics(registers, programMemory, &programCounter);
}

// test result wrapper
void assertTest(const char *testName, int conditioNULL)
{
	testsRun++;
	if (conditioNULL)
	{
		testsPassed++;
		printf("  PASS: %s\n", testName);
	}
	else
	{
		printf("  FAIL: %s\n", testName);
	}
}

// test loading files
void modifiedLoadFile(char *inputPath)
{
	FILE *inputBinaryFile;
	int single_byte;
	int position = startingAddress;
	if ((inputBinaryFile = fopen(inputPath, "rb")) != NULL)
	{
		while ((single_byte = fgetc(inputBinaryFile)) != EOF)
		{
			programMemory[position] = single_byte;
			position++;
			if (position >= MEM_SIZE)
			{
				throwError("Simulation error");
			}
		}
	}
	else
	{
		fprintf(stderr, "Warning: Could not open test file '%s', skipping file load test\n", inputPath);
		return;
	}
	testFileLoaded(position, inputPath);
	fclose(inputBinaryFile);
}

// second part of file loading test, loop through and double check read correctly
void testFileLoaded(int position, char *inputPath)
{
	FILE *inputBinaryFile = fopen(inputPath, "rb");
	if (inputBinaryFile == NULL)
	{
		printf("  FAIL: testFileLoaded - could not reopen file\n");
		testsRun++;
		return;
	}
	int single_byte;
	int i = startingAddress;
	int match = 1;
	while ((single_byte = fgetc(inputBinaryFile)) != EOF)
	{
		if (programMemory[i] != (unsigned char)single_byte)
		{
			match = 0;
			printf("  FAIL: testFileLoaded");
			break;
		}
		i++;
	}
	fclose(inputBinaryFile);
	assertTest("File bytes loaded correctly into programMemory", match && i == position);
}

// test instructions
void testAnd()
{
	printf("\nAND\n");
	resetState();
	registers[1] = 0xff0ff0ffff00f000;
	registers[2] = 0xf0f0f0f0f0f0f0f0;
	and(3, 1, 2, 0);
	assertTest("r3 = r1 & r2", registers[3] == (0xf0f0f0f0f0f0f0f0 & 0xff0ff0ffff00f000));

	resetState();
}

void testOr()
{
	printf("\nOR\n");
	resetState();
	registers[1] = 0xFF00FF00FF00FF00;
	registers[2] = 0x00FF00FF00FF00FF;
	or(3, 1, 2, 0);
	assertTest("r3 = r1 | r2", registers[3] == 0xFFFFFFFFFFFFFFFF);
}

void testXor()
{
	printf("\nXOR\n");
	resetState();
	registers[1] = 0xAAAAAAAAAAAAAAAA;
	registers[2] = 0x5555555555555555;
	xor(3, 1, 2, 0);
	assertTest("r3 = r1 ^ r2 (all different bits)", registers[3] == 0xFFFFFFFFFFFFFFFF);
}

void testNot()
{
	printf("\nNOT\n");
	resetState();
	registers[1] = 0;
	not(3, 1, 0, 0);
	assertTest("~0 = all ones", registers[3] == 0xFFFFFFFFFFFFFFFF);
}

void testShftr()
{
	printf("\nSHFTR\n");
	resetState();
	registers[1] = 0x80;
	registers[2] = 4;
	shftr(3, 1, 2, 0);
	assertTest("0x80 >> 4 = 0x08", registers[3] == 0x08);
}

void testShftri()
{
	printf("\nSHFTRI\n");
	resetState();
	registers[3] = 0xFF00;
	shftri(3, 0, 0, 8);
	assertTest("0xFF00 >> 8 = 0xFF", registers[3] == 0xFF);
}

void testShftl()
{
	printf("\nSHFTL\n");
	resetState();
	registers[1] = 1;
	registers[2] = 16;
	shftl(3, 1, 2, 0);
	assertTest("1 << 16 = 0x10000", registers[3] == 0x10000);
}

void testShftli()
{
	printf("\nSHFTLI\n");
	resetState();
	registers[3] = 1;
	shftli(3, 0, 0, 10);
	assertTest("1 << 10 = 1024", registers[3] == 1024);
}

void testBr()
{
	printf("\nBR\n");
	resetState();
	registers[5] = 0x2000;
	br(5, 0, 0, 0);
	assertTest("PC = r5 = 0x2000", programCounter == 0x2000);
}

void testBrrReg()
{
	printf("\nBRR_REG\n");
	resetState();
	registers[5] = 0x100;
	brr_reg(5, 0, 0, 0);
	assertTest("PC += r5 (0x1000 + 0x100 = 0x1100)", programCounter == 0x1100);
}

void testBrrLit()
{
	printf("\nBRR_LIT\n");
	resetState();
	brr_lit(0, 0, 0, 0x10);
	assertTest("PC += 16 (positive literal)", programCounter == startingAddress + 16);
}

void testBrnz()
{
	printf("\nBRNZ\n");
	// rs != 0 --> branch
	resetState();
	registers[5] = 0x2000;
	registers[1] = 42;
	brnz(5, 1, 0, 0);
	assertTest("rs != 0: PC = rd", programCounter == 0x2000);

	// rs == 0 --> no branch
	resetState();
	registers[5] = 0x2000;
	registers[1] = 0;
	brnz(5, 1, 0, 0);
	assertTest("rs == 0: PC += 4", programCounter == startingAddress + 4);
}

void testBrgt()
{
	printf("\nBRGT\n");
	// rs > rt -> branch
	resetState();
	registers[5] = 0x3000;
	registers[1] = 100;
	registers[2] = 50;
	brgt(5, 1, 2, 0);
	assertTest("rs > rt: PC = rd", programCounter == 0x3000);

	// rs <= rt -> no branch
	resetState();
	registers[5] = 0x3000;
	registers[1] = 50;
	registers[2] = 100;
	brgt(5, 1, 2, 0);
	assertTest("rs <= rt: PC += 4", programCounter == startingAddress + 4);
}

void testCall()
{
	printf("\nCALL\n");
	resetState();
	registers[5] = 0x5000;
	uint64_t oldSP = registers[31];
	call(5, 0, 0, 0);

	// return address (old PC + 4) should be stored at SP - 8
	uint64_t storedRetAddr;
	memcpy(&storedRetAddr, &programMemory[oldSP - 8], sizeof(uint64_t));
	assertTest("Return address stored in memory", storedRetAddr == (uint64_t)(startingAddress + 4));
	assertTest("PC = rd = 0x5000", programCounter == 0x5000);
}

void testReturn()
{
	printf("\nRETURN\n");
	resetState();
	// simulate a call first, then return
	registers[5] = 0x5000;
	uint64_t oldSP = registers[31];
	call(5, 0, 0, 0);

	// now return should restore PC from stack
	run_return(0, 0, 0, 0);
	assertTest("Return restores PC to original + 4", programCounter == (int64_t)(startingAddress + 4));
}

void testMovReg()
{
	printf("\nMOV_REG\n");
	resetState();
	registers[1] = 0xDEADBEEFCAFEBABE;
	mov_reg(2, 1, 0, 0);
	assertTest("r2 = r1", registers[2] == 0xDEADBEEFCAFEBABE);
}

void testMovLit()
{
	printf("\nMOV_LIT\n");
	resetState();
	registers[3] = 0xFFFFFFFFFFFFFFFF;
	mov_lit(3, 0, 0, 0x123);
	assertTest("Lower 12 bits set to literal", (registers[3] & 0xFFF) == 0x123);
	assertTest("Upper bits preserved", (registers[3] & 0xFFFFFFFFFFFFF000) == 0xFFFFFFFFFFFFF000);
}

void testMovLoad()
{
	printf("\nMOV_LOAD\n");
	resetState();
	// store a known value in memory at address 0x2000
	uint64_t testVal = 0x1234567890ABCDEF;
	memcpy(&programMemory[0x2000], &testVal, sizeof(uint64_t));
	registers[1] = 0x2000;
	mov_load(3, 1, 0, 0); // L=0, load from address in r1
	assertTest("Load 8 bytes from memory", registers[3] == testVal);
}

void testMovStore()
{
	printf("\nMOV_STORE\n");
	resetState();
	registers[1] = 0xCAFEBABEDEADBEEF;
	registers[5] = 0x3000;
	mov_store(5, 1, 0, 0); // store r1 at address in r5
	uint64_t readBack;
	memcpy(&readBack, &programMemory[0x3000], sizeof(uint64_t));
	assertTest("Store 8 bytes to memory", readBack == 0xCAFEBABEDEADBEEF);
}

void testAdd()
{
	printf("\nADD\n");
	resetState();
	registers[1] = 100;
	registers[2] = 200;
	add(3, 1, 2, 0);
	assertTest("100 + 200 = 300", registers[3] == 300);
}

void testAddi()
{
	printf("\nADDI\n");
	resetState();
	registers[3] = 1000;
	addi(3, 0, 0, 42);
	assertTest("1000 + 42 = 1042", registers[3] == 1042);
}

void testSub()
{
	printf("\nSUB\n");
	resetState();
	registers[1] = 500;
	registers[2] = 200;
	sub(3, 1, 2, 0);
	assertTest("500 - 200 = 300", registers[3] == 300);
}

void testSubi()
{
	printf("\nSUBI\n");
	resetState();
	registers[3] = 1000;
	subi(3, 0, 0, 42);
	assertTest("1000 - 42 = 958", registers[3] == 958);
}

void testMul()
{
	printf("\nMUL\n");
	resetState();
	registers[1] = 7;
	registers[2] = 8;
	mul(3, 1, 2, 0);
	assertTest("7 * 8 = 56", registers[3] == 56);
}

void testDiv()
{
	printf("\nDIV\n");
	resetState();
	registers[1] = 100;
	registers[2] = 10;
	run_div(3, 1, 2, 0);
	assertTest("100 / 10 = 10", registers[3] == 10);
}

void testAddf()
{
	printf("\nADDF\n");
	resetState();
	double a = 1.5, b = 2.5;
	memcpy(&registers[1], &a, sizeof(double));
	memcpy(&registers[2], &b, sizeof(double));
	addf(3, 1, 2, 0);
	double result;
	memcpy(&result, &registers[3], sizeof(double));
	assertTest("1.5 + 2.5 = 4.0", result == 4.0);
}

void testSubf()
{
	printf("\nSUBF\n");
	resetState();
	double a = 7.0, b = 3.5;
	memcpy(&registers[1], &a, sizeof(double));
	memcpy(&registers[2], &b, sizeof(double));
	subf(3, 1, 2, 0);
	double result;
	memcpy(&result, &registers[3], sizeof(double));
	assertTest("7.0 - 3.5 = 4.5", result == 4.5);
}

void testMulf()
{
	printf("\nMULF\n");
	resetState();
	double a = 3.0, b = 4.0;
	memcpy(&registers[1], &a, sizeof(double));
	memcpy(&registers[2], &b, sizeof(double));
	mulf(3, 1, 2, 0);
	double result;
	memcpy(&result, &registers[3], sizeof(double));
	assertTest("3.0 * 4.0 = 12.0", result == 12.0);
}

void testDivf()
{
	printf("\nDIVF\n");
	resetState();
	double a = 10.0, b = 4.0;
	memcpy(&registers[1], &a, sizeof(double));
	memcpy(&registers[2], &b, sizeof(double));
	divf(3, 1, 2, 0);
	double result;
	memcpy(&result, &registers[3], sizeof(double));
	assertTest("10.0 / 4.0 = 2.5", result == 2.5);
}

int main(int argc, char *argv[])
{
	printf("tests\n");
	testAnd();
	testOr();
	testXor();
	testNot();
	testShftr();
	testShftri();
	testShftl();
	testShftli();
	testBr();
	testBrrReg();
	testBrrLit();
	testBrnz();
	testBrgt();
	testCall();
	testReturn();
	testMovReg();
	testMovLit();
	testMovLoad();
	testMovStore();
	testAdd();
	testAddi();
	testSub();
	testSubi();
	testMul();
	testDiv();
	testAddf();
	testSubf();
	testMulf();
	testDivf();

	printf("\nyay %d/%d tests passed\n", testsPassed, testsRun);
	return 0;
}
