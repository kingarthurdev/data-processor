#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArrayList.h"

#define IS_CURRENTLY_TESTING 0
typedef struct testingReturnType
{
	char *line;
	int type; // 0 for data type, 1 for code type, 2 for label, -1 for neither
} testingReturnType;

// prior defs:
testingReturnType processLine(char *lineInput, int outputRun);
void processInputFile(char *inputFilePath);
void throwError(char *message);
void processInstructions(testingReturnType *input, char *lineInput);
int startsWith(char *searchString, char *string);
int startsWith2(char *inputString, char *searchStrings[], int numStrings);
void stripChars(char *input, char charToStrip);
int validateNumArgs(char *inputs, int numExpectedArgs);
void handleValidateNumArgs(char *inputs, int numExpectedArgs);
void incrementBytes(int type);
void runTests();

char *convertHaltMacro(char *input);
char *convertClrMacro(char *input);
char *convertLdMacro(char *input);
char *convertInMacro(char *input);
char *convertOutMacro(char *input);
char *convertPushMacro(char *input);
char *convertPopMacro(char *input);

PDArrayList listOfLabels;
const char delimiters[] = ", ";

// initialize some "string builders" and hope 5k chars is enough
char code[5000] = {0};
char data[5000] = {0};

// 0 for data mode, 1 for int, -1 for unset
int type = -1;

// 0 for false, 1 for true
int hasSeenAtLeast1Code = 0;

// count # of prior instructions (starting at 0x1000 since that's where the program starts)
long long memNumBytes = 0x1000;

int numRuns = 0;

int main(int argc, char *argv[])
{

	char *inputFilePath = argv[1];
	char *intermediateOutputFilePath = argv[2];
	char *binaryOutputFilePath = argv[3];
	newPDArrayList(&listOfLabels);

	if (!IS_CURRENTLY_TESTING)
	{
		processInputFile(inputFilePath);
	}
	else
	{
		processInputFile(inputFilePath);
		runTests();
	}

	return 0;
}

void processInputFile(char *inputFilePath)
{
	FILE *file;
	if ((file = fopen(inputFilePath, "r")) != NULL)
	{
		int BUFFER_SIZE = 1024; // Accepting 1024 chars in case they feed in a decently long line
		char line[1024];
		// first run
		while (fgets(line, BUFFER_SIZE, file) != NULL)
		{
			processLine(line, 0);
		}

		// run again to actually write the data after the symbol table has been generated
		while (fgets(line, BUFFER_SIZE, file) != NULL)
		{
			processLine(line, 1);
		}
	}
	else
	{
		throwError("ERROR! INVALID FILE INPUT PASSED (cannot access file)!");
	}
}
//output run should be 0 if this is just the first pass, 1 if final passs
testingReturnType processLine(char *lineInput, int outputRun)
{
	testingReturnType tempThingy;
	if ((lineInput[0]) == '\t') // should be instruction
	{
		if (type == -1)
		{
			throwError("ERROR! DATA/INSTRUCTION TYPE UNDEFINED!");
		}
		processInstructions(&tempThingy, lineInput);
	}
	else if ((lineInput[0]) == ':') // label
	{
		tempThingy.type = 2;
		tempThingy.line = "";
		PD newPD;
		char buffer[7] = {0};
		sprintf(buffer, "0x%x", memNumBytes);
		newPD.replacementString = buffer;
		addPD(&listOfLabels, &newPD);
		// todo: add to symbol table (like hash table) and then do like a preprocessor directive cleanup at the very end once all the things have been resolved.
		// lowk too lazy to make hashtable
	}
	else if ((lineInput[0]) == ';') // comment, so ignore
	{
		tempThingy.type = -1;
		tempThingy.line = "";
	}
	else if ((lineInput[0]) == '.')
	{ // either .code or .data
		if (strncmp(lineInput, ".data", 5) == 0)
		{
			tempThingy.type = 0;
			type = 0;
		}
		else if (strncmp(lineInput, ".code", 5) == 0)
		{
			tempThingy.type = 1;
			type = 1;
			hasSeenAtLeast1Code = 1;
		}
		else
		{
			throwError("ERROR! INVALID LINE STARTING WITH DOT!");
		}
	}
	else
	{
		throwError("ERROR! UNRECOGNIZED LINE FORMAT!");
	}

	return tempThingy;
}

void throwError(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

void processInstructions(testingReturnType *input, char *lineInput)
{
	char *threeByThrees[] = {"add", "sub", "mul", "div", "xor"};
	char *fourByTwo[] = {"addi", "subi", "brnz"};
	if (startsWith2(lineInput, threeByThrees, 5))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
	}
	else if (startsWith2(lineInput, fourByTwo, 3))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 2);
		// rd <- rd + L (Opcode 0x19)
	}
	else if (startsWith("call", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 1);
		// pc <- rd, saves return addr (Opcode 0xc)
	}

	else if (startsWith("or", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 3);
		// rd <- rs | rt (Opcode 0x1f)
	}
	else if (startsWith("shftli", lineInput))
	{
		handleValidateNumArgs(lineInput + 6 + 1, 2);
		// rd <- rd << L (Opcode 0x22)
	}
	else if (startsWith("shftl", lineInput))
	{
		handleValidateNumArgs(lineInput + 5 + 1, 3);
		// rd <- rs << rt (Opcode 0x21)
	}
	else if (startsWith("shftri", lineInput))
	{
		handleValidateNumArgs(lineInput + 6 + 1, 2);
		// rd <- rd >> L (Opcode 0x24)
	}
	else if (startsWith("shftr", lineInput))
	{
		handleValidateNumArgs(lineInput + 5 + 1, 3);
		// rd <- rs >> rt (Opcode 0x23)
	}
	else if (startsWith("mov", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		// Could be:
		// 1. mov rd, rs (Reg-to-Reg)
		// 2. mov rd, (rs)(L) (Load from memory)
		// 3. mov (rd)(L), rs (Store to memory)
	}
	else if (startsWith("brr", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 1);
		// NOTE: can be brr r_d OR brr L
		//  Branch Relative: PC <- PC + L (Opcode 0x40)
	}
	else if (startsWith("brgt", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		// pc <- rd if rs > rt (Opcode 0xe)
	}
	else if (startsWith("br", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 1);
		// pc <- rd (Opcode 0x8)
	}
	else if (startsWith("return", lineInput))
	{
		// No args (Opcode 0xd)
	}
	else if (startsWith("addf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		// Opcode 0x14
	}
	else if (startsWith("subf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		// Opcode 0x15
	}
	else if (startsWith("mulf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		// Opcode 0x16
	}
	else if (startsWith("divf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		// Opcode 0x17
	}
	else if (startsWith("and", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 3);
		// rd <- rs & rt (Opcode 0x0)
	}
	else if (startsWith("not", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		// rd <- ~rs (Opcode 0x3)
	}
	else if (startsWith("priv", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 4);
		// priv rd, rs, rt, L (Opcode 0xf)
	}
	// === MACROS (to be expanded later) ===
	else if (startsWith("clr", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 1);
		// Macro: expands to xor rd, rd, rd
	}
	else if (startsWith("ld", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 2);
		// Macro: expands to xor + addi/shftli chain
	}
	else if (startsWith("halt", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 0);
		// Macro: no args, expands to priv r0, r0, r0, 0x0
	}
	else if (startsWith("in", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 2);
		// Macro: expands to priv rd, rs, r0, 0x3
	}
	else if (startsWith("out", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		// Macro: expands to priv rd, rs, r0, 0x4
	}
	else if (startsWith("push", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 1);
		// Macro: push rd onto stack
	}
	else if (startsWith("pop", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 1);
		// Macro: pop into rd from stack
	}
	else
	{
		throwError("ERROR! INVALID INSTRUCTION!");
	}
}

//todo: implement
void storePDs()
{
}

// Return 1 if true, 0 if false, NOTE: I added an extra +1 since I'm accounting for tab indentation starts
int startsWith(char *searchString, char *string)
{
	return !strncmp(searchString, string + 1, strlen(searchString));
}

void stripChars(char *input, char charToStrip)
{
	int counterIndex = 0;
	for (int i = 0; i < strlen(input); i++)
	{
		if (input[i] != charToStrip)
		{
			input[counterIndex] = input[i];
			counterIndex++;
		}
	}
	input[counterIndex] = '\0';
}

// exists for testing purposes
int validateNumArgs(char *inputs, int numExpectedArgs)
{
	char *token = strtok(inputs, delimiters);

	int count = 0;
	while (1)
	{
		if (token != NULL)
		{
			count++;
		}
		else
		{
			break;
		}
		token = strtok(NULL, delimiters);
	}
	return count == numExpectedArgs;
}

// checks that num args is what is expcected -- but the extra +1 usually added is to handle that pesky tab char
void handleValidateNumArgs(char *inputs, int numExpectedArgs)
{
	if (validateNumArgs(inputs, numExpectedArgs) == 0)
	{
		throwError("ERROR! INVALID SYNTAX FOR INSTRUCTION PASSED!");
	}
	incrementBytes(type);
}

// Return 1 if true, 0 if false, NOTE: extra +1 added to account for tab indent
int startsWith2(char *inputString, char *searchStrings[], int numStrings)
{
	for (int i = 0; i < numStrings; i++)
	{
		if (strncmp(inputString + 1, searchStrings[i], strlen(searchStrings[i])) == 0)
		{
			return 1;
		}
	}
	return 0;
}

void incrementBytes(int type)
{
	if (type == 0)
	{
		memNumBytes += 8;
	}
	else if (type == 1)
	{
		memNumBytes += 4;
	}
	else
	{
		throwError("ERROR! Instruction called before .code or .data");
	}
}

long long evaluateLabel(char *label)
{
	for (int i = 0; i < listOfLabels.size; i++)
	{
		if (strcmp(label, listOfLabels.data[i]->thingToReplace) == 0)
		{
			return listOfLabels.data[i]-> replacementInt;
		}
	}
	throwError("ERROR! LABEL NOT RECOGNIZED!");
}

// Test functions:
void runTests()
{
	printf("TESTING MODE ACTIVE!");
}

// halt -> priv r0, r0, r0, 0x0
char *convertHaltMacro(char *input)
{
	return "priv r0, r0, r0, 0x0";
}

// clr rd -> xor rd, rd, rd
char *convertClrMacro(char *input)
{
	char *buffer = malloc(sizeof(char)*10); // more than enough space
	input += 4;			   // to get rid of the tab + clr
	char *token = strtok(input, delimiters);
	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);
	return buffer;
}

//TODO: increment up the memNumBytes when I do all this macro expansion so we can accurately eval stuff
// ld rd, L -> series of mov and shftli instructions to load 64-bit value
char *convertLdMacro(char *input)
{
	// int isZeros = 1; //means we're just starting off, leading bits are zero so can be discarded
	char *buffer = malloc(sizeof(char)*50); // more than enough space
	input += 3;				// to get rid of the tab + clr
	long long value = 0;

	char *token = strtok(input, delimiters); // the register
	char *token2 = strtok(NULL, delimiters); // the data -- either a label or a literal\

	if(token2[0] == ':'){
		if(numRuns == 0){ //first pass, so inconsequential
			value = 0;
		}else{
			value = evaluateLabel(token2);
		}
	}else{
		value = strtoll(token2, NULL, 10);
	}

	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value >> 60) & 0xF);
	sprintf(buffer, "\n\tshftli %s, 12", token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value >> 48) & 0xFFF);
	sprintf(buffer, "\n\tshftli %s, 12", token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value >> 36) & 0xFFF);
	sprintf(buffer, "\n\tshftli %s, 12", token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value >> 24) & 0xFFF);
	sprintf(buffer, "\n\tshftli %s, 12", token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value >> 12) & 0xFFF);
	sprintf(buffer, "\n\tshftli %s, 12", token);

	sprintf(buffer, "\n\taddi %s, %li", token, (value & 0xFFF));

	memNumBytes += 11; //one less than actual since the data validation will add one //TODO: fix if we update the structure


	// so shift over by 60 --> mask with 0xF
	// then shift over by 12 --> mask with 0xFFF
	// repeatedly shift over by 12 until all done

	// TODO: parse rd and L from input
	// If L is a label, resolve it first
	// Return sequence: xor rd, rd, rd then mov/shftli chain
	return buffer;
}

// in rd, rs -> priv rd, rs, r0, 0x3
char *convertInMacro(char *input)
{
	// TODO: parse rd, rs from input, return "priv rd, rs, r0, 0x3"
	return NULL;
}

// out rd, rs -> priv rd, rs, r0, 0x4
char *convertOutMacro(char *input)
{
	// TODO: parse rd, rs from input, return "priv rd, rs, r0, 0x4"
	return NULL;
}

// push rd -> subi r31, 8; mov (r31)(0), rd
char *convertPushMacro(char *input)
{
	// TODO: parse rd from input
	// Return two instructions: "subi r31, 8" and "mov (r31)(0), rd"
	return NULL;
}

// pop rd -> mov rd, (r31)(0); addi r31, 8
char *convertPopMacro(char *input)
{
	// TODO: parse rd from input
	// Return two instructions: "mov rd, (r31)(0)" and "addi r31, 8"
	return NULL;
}