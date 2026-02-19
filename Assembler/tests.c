#include "main.h"

// Test functions:

void runTests()
{
	printf("TESTING MODE ACTIVE!\n");
	int passed = 0;
	int failed = 0;

	// TEST parseReg
	if (parseReg("r0") == 0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg r0\n");
	}
	if (parseReg("r1") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg r1\n");
	}
	if (parseReg("r15") == 15)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg r15\n");
	}
	if (parseReg("r31") == 31)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg r31\n");
	}
	if (parseReg("R0") == 0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg R0\n");
	}
	if (parseReg("R31") == 31)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parseReg R31\n");
	}

	// TEST getExpectedArgCount
	if (getExpectedArgCount(FMT_RRR) == 3)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_RRR\n");
	}
	if (getExpectedArgCount(FMT_RR) == 2)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_RR\n");
	}
	if (getExpectedArgCount(FMT_RL) == 2)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_RL\n");
	}
	if (getExpectedArgCount(FMT_R) == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_R\n");
	}
	if (getExpectedArgCount(FMT_L) == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_L\n");
	}
	if (getExpectedArgCount(FMT_NONE) == 0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_NONE\n");
	}
	if (getExpectedArgCount(FMT_RRRL) == 4)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_RRRL\n");
	}
	if (getExpectedArgCount(FMT_MOV) == 2)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_MOV\n");
	}
	if (getExpectedArgCount(FMT_R_OR_L) == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getExpectedArgCount FMT_R_OR_L\n");
	}

	// TEST startsWith
	if (startsWith("add", "\tadd r1, r2, r3") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith add\n");
	}
	if (startsWith("sub", "\tsub r1, r2, r3") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith sub\n");
	}
	if (startsWith("addi", "\taddi r1, 5") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith addi\n");
	}
	if (startsWith("add", "\tsub r1, r2, r3") == 0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith add != sub\n");
	}
	if (startsWith("mov", "\tmov r1, r2") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith mov\n");
	}
	if (startsWith("brr", "\tbrr r1") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: startsWith brr\n");
	}

	// TEST convertNone
	uint32_t expected = 0xd << 27;
	if (convertNone(0xd) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertNone 0xd\n");
	}
	expected = 0x0 << 27;
	if (convertNone(0x0) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertNone 0x0\n");
	}

	// TEST convertR
	expected = (0x8 << 27) | (5 << 22);
	if (convertR(0x8, 5) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertR br r5\n");
	}
	expected = (0x9 << 27) | (31 << 22);
	if (convertR(0x9, 31) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertR brr r31\n");
	}
	expected = (0xc << 27) | (0 << 22);
	if (convertR(0xc, 0) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertR call r0\n");
	}

	// TEST convertRR
	expected = (0x3 << 27) | (1 << 22) | (2 << 17);
	if (convertRR(0x3, 1, 2) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRR not r1 r2\n");
	}
	expected = (0xb << 27) | (10 << 22) | (20 << 17);
	if (convertRR(0xb, 10, 20) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRR brnz r10 r20\n");
	}

	// TEST convertRL
	expected = (0x19 << 27) | (5 << 22) | (100 & 0xFFF);
	if (convertRL(0x19, 5, 100) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRL addi r5 100\n");
	}
	expected = (0x1b << 27) | (3 << 22) | (4095 & 0xFFF);
	if (convertRL(0x1b, 3, 4095) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRL subi r3 4095\n");
	}
	expected = (0x5 << 27) | (7 << 22) | (12 & 0xFFF);
	if (convertRL(0x5, 7, 12) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRL shftri r7 12\n");
	}
	expected = (0x7 << 27) | (8 << 22) | (8 & 0xFFF);
	if (convertRL(0x7, 8, 8) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRL shftli r8 8\n");
	}

	// TEST convertRRR
	expected = (0x18 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0x18, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR add r1 r2 r3\n");
	}
	expected = (0x1a << 27) | (4 << 22) | (5 << 17) | (6 << 12);
	if (convertRRR(0x1a, 4, 5, 6) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR sub r4 r5 r6\n");
	}
	expected = (0x1c << 27) | (7 << 22) | (8 << 17) | (9 << 12);
	if (convertRRR(0x1c, 7, 8, 9) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR mul r7 r8 r9\n");
	}
	expected = (0x1d << 27) | (10 << 22) | (11 << 17) | (12 << 12);
	if (convertRRR(0x1d, 10, 11, 12) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR div r10 r11 r12\n");
	}
	expected = (0x0 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0x0, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR and r1 r2 r3\n");
	}
	expected = (0x1 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0x1, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR or r1 r2 r3\n");
	}
	expected = (0x2 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0x2, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR xor r1 r2 r3\n");
	}
	expected = (0xe << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0xe, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR brgt r1 r2 r3\n");
	}
	expected = (0x14 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (convertRRR(0x14, 1, 2, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRR addf r1 r2 r3\n");
	}

	// TEST convertL
	expected = (0xa << 27) | (50 & 0xFFF);
	if (convertL(0xa, 50) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertL brr 50\n");
	}
	expected = (0xa << 27) | ((-100) & 0xFFF);
	if (convertL(0xa, -100) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertL brr -100\n");
	}
	expected = (0xa << 27) | (2047 & 0xFFF);
	if (convertL(0xa, 2047) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertL brr 2047\n");
	}
	expected = (0xa << 27) | ((-2048) & 0xFFF);
	if (convertL(0xa, -2048) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertL brr -2048\n");
	}

	// TEST convertRRRL
	expected = (0xf << 27) | (0 << 22) | (0 << 17) | (0 << 12) | (0 & 0xFFF);
	if (convertRRRL(0xf, 0, 0, 0, 0) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRRL priv halt\n");
	}
	expected = (0xf << 27) | (1 << 22) | (2 << 17) | (0 << 12) | (3 & 0xFFF);
	if (convertRRRL(0xf, 1, 2, 0, 3) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRRL priv in\n");
	}
	expected = (0xf << 27) | (1 << 22) | (2 << 17) | (0 << 12) | (4 & 0xFFF);
	if (convertRRRL(0xf, 1, 2, 0, 4) == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertRRRL priv out\n");
	}

	// TEST convertMOV 0x11 (mov rd, rs)
	expected = (0x11 << 27) | (1 << 22) | (2 << 17);
	if (convertMOV(0x11, "mov r1, r2") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r1 r2\n");
	}
	expected = (0x11 << 27) | (31 << 22) | (0 << 17);
	if (convertMOV(0x11, "mov r31, r0") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r31 r0\n");
	}

	// TEST convertMOV 0x12 (mov rd, L)
	expected = (0x12 << 27) | (5 << 22) | (100 & 0xFFF);
	if (convertMOV(0x12, "mov r5, 100") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r5 100\n");
	}
	expected = (0x12 << 27) | (10 << 22) | (4095 & 0xFFF);
	if (convertMOV(0x12, "mov r10, 4095") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r10 4095\n");
	}

	// TEST convertMOV 0x10 (mov rd, (rs)(L))
	expected = (0x10 << 27) | (1 << 22) | (2 << 17) | (8 & 0xFFF);
	if (convertMOV(0x10, "mov r1, (r2)(8)") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r1 (r2)(8)\n");
	}
	expected = (0x10 << 27) | (5 << 22) | (31 << 17) | (0 & 0xFFF);
	if (convertMOV(0x10, "mov r5, (r31)(0)") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r5 (r31)(0)\n");
	}
	expected = (0x10 << 27) | (1 << 22) | (2 << 17) | ((-8) & 0xFFF);
	if (convertMOV(0x10, "mov r1, (r2)(-8)") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov r1 (r2)(-8)\n");
	}

	// TEST convertMOV 0x13 (mov (rd)(L), rs)
	expected = (0x13 << 27) | (31 << 22) | (1 << 17) | ((-8) & 0xFFF);
	if (convertMOV(0x13, "mov (r31)(-8), r1") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov (r31)(-8) r1\n");
	}
	expected = (0x13 << 27) | (2 << 22) | (3 << 17) | (16 & 0xFFF);
	if (convertMOV(0x13, "mov (r2)(16), r3") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: convertMOV mov (r2)(16) r3\n");
	}

	// TEST parse64BitNums
	if (parse64BitNums("0") == 0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums 0\n");
	}
	if (parse64BitNums("1") == 1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums 1\n");
	}
	if (parse64BitNums("255") == 255)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums 255\n");
	}
	if (parse64BitNums("0xFF") == 255)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums 0xFF\n");
	}
	if (parse64BitNums("0x1000") == 4096)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums 0x1000\n");
	}
	if (parse64BitNums("18446744073709551615") == 18446744073709551615ULL)
		passed++;
	else
	{
		failed++;
		printf("FAIL: parse64BitNums max\n");
	}

	// TEST getOpcode
	if (getOpcode("and r1, r2, r3") == 0x0)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode and\n");
	}
	if (getOpcode("or r1, r2, r3") == 0x1)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode or\n");
	}
	if (getOpcode("xor r1, r2, r3") == 0x2)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode xor\n");
	}
	if (getOpcode("not r1, r2") == 0x3)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode not\n");
	}
	if (getOpcode("shftr r1, r2, r3") == 0x4)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode shftr\n");
	}
	if (getOpcode("shftri r1, 5") == 0x5)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode shftri\n");
	}
	if (getOpcode("shftl r1, r2, r3") == 0x6)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode shftl\n");
	}
	if (getOpcode("shftli r1, 5") == 0x7)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode shftli\n");
	}
	if (getOpcode("br r1") == 0x8)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode br\n");
	}
	if (getOpcode("brr r1") == 0x9)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode brr reg\n");
	}
	if (getOpcode("brr 100") == 0xa)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode brr lit\n");
	}
	if (getOpcode("brr -50") == 0xa)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode brr neg\n");
	}
	if (getOpcode("brnz r1, r2") == 0xb)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode brnz\n");
	}
	if (getOpcode("call r1") == 0xc)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode call\n");
	}
	if (getOpcode("return") == 0xd)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode return\n");
	}
	if (getOpcode("brgt r1, r2, r3") == 0xe)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode brgt\n");
	}
	if (getOpcode("priv r0, r0, r0, 0") == 0xf)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode priv\n");
	}
	if (getOpcode("mov r1, (r2)(0)") == 0x10)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mov mr\n");
	}
	if (getOpcode("mov r1, r2") == 0x11)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mov rr\n");
	}
	if (getOpcode("mov r1, 100") == 0x12)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mov rl\n");
	}
	if (getOpcode("mov (r1)(0), r2") == 0x13)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mov rm\n");
	}
	if (getOpcode("addf r1, r2, r3") == 0x14)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode addf\n");
	}
	if (getOpcode("subf r1, r2, r3") == 0x15)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode subf\n");
	}
	if (getOpcode("mulf r1, r2, r3") == 0x16)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mulf\n");
	}
	if (getOpcode("divf r1, r2, r3") == 0x17)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode divf\n");
	}
	if (getOpcode("add r1, r2, r3") == 0x18)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode add\n");
	}
	if (getOpcode("addi r1, 5") == 0x19)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode addi\n");
	}
	if (getOpcode("sub r1, r2, r3") == 0x1a)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode sub\n");
	}
	if (getOpcode("subi r1, 5") == 0x1b)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode subi\n");
	}
	if (getOpcode("mul r1, r2, r3") == 0x1c)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode mul\n");
	}
	if (getOpcode("div r1, r2, r3") == 0x1d)
		passed++;
	else
	{
		failed++;
		printf("FAIL: getOpcode div\n");
	}

	// TEST processIntermediateLine
	expected = (0x18 << 27) | (1 << 22) | (2 << 17) | (3 << 12);
	if (processIntermediateLine("add r1, r2, r3") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: processIntermediateLine add\n");
	}
	expected = (0xd << 27);
	if (processIntermediateLine("return") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: processIntermediateLine return\n");
	}
	expected = (0x19 << 27) | (5 << 22) | (100 & 0xFFF);
	if (processIntermediateLine("addi r5, 100") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: processIntermediateLine addi\n");
	}
	expected = (0xa << 27) | ((-4) & 0xFFF);
	if (processIntermediateLine("brr -4") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: processIntermediateLine brr\n");
	}
	expected = (0xf << 27) | (0 << 22) | (0 << 17) | (0 << 12) | (0 & 0xFFF);
	if (processIntermediateLine("priv r0, r0, r0, 0") == expected)
		passed++;
	else
	{
		failed++;
		printf("FAIL: processIntermediateLine priv halt\n");
	}

	printf("TESTS PASSED: %d\n", passed);
	printf("TESTS FAILED: %d\n", failed);
}
