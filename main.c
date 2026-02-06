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
testingReturnType processLine(char *lineInput);
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
char *parseAndFormatArgs(char *inputString);
void autoAdd(char *inputString);
void autoAddMacro(char *inputString);

PDArrayList listOfLabels;
const char delimiters[] = ", \t\n\r";

// initialize some "string builders" and hope 5k chars is enough
char code[5000] = {0};
char data[5000] = {0};

char codeAndDataCombined[10000] = {0}; // used for final output

// 0 for data mode, 1 for int, -1 for unset
int type = -1;

// 0 for false, 1 for true
int hasSeenAtLeast1Code = 0;

// count # of prior instructions (starting at 0x1000 since that's where the program starts)
long long memNumBytes = 0x1000;

int numRuns = 0; // 0 for the first run, 1 for the next (so we can preeval all the labels)

int debuggingLineCount = 0;

int isFirstLineInOutput = 1;

int main(int argc, char *argv[])
{

	char *inputFilePath = argv[1];
	char *intermediateOutputFilePath = argv[2];
	char *binaryOutputFilePath = argv[3];
	newPDArrayList(&listOfLabels);

	if (!IS_CURRENTLY_TESTING)
	{
		processInputFile(inputFilePath);
		printf("%s", codeAndDataCombined);
	}
	else
	{
		processInputFile(inputFilePath);
		runTests();
		printf("%s", codeAndDataCombined);
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
			processLine(line);
		}

		numRuns++;
		code[0] = '\0';
		data[0] = '\0';
		codeAndDataCombined[0] = '\0';
		isFirstLineInOutput = 1;
		rewind(file);
		type = -1;
		memNumBytes = 0x1000;

		// run again to actually write the data after the symbol table has been generated
		while (fgets(line, BUFFER_SIZE, file) != NULL)
		{
			processLine(line);
		}

		// put the last bit in
		if (type == 1 && code[0] != '\0')
		{
			if (isFirstLineInOutput)
			{
				strcat(codeAndDataCombined, ".code");
				isFirstLineInOutput = 0;
			}
			else
			{
				strcat(codeAndDataCombined, "\n.code");
			}
			strcat(codeAndDataCombined, code);
		}
		else if (type == 0 && data[0] != '\0')
		{
			if (isFirstLineInOutput)
			{
				strcat(codeAndDataCombined, ".data");
				isFirstLineInOutput = 0;
			}
			else
			{
				strcat(codeAndDataCombined, "\n.data");
			}
			strcat(codeAndDataCombined, data);
		}
	}
	else
	{
		throwError("ERROR! INVALID FILE INPUT PASSED (cannot access file)!");
	}
}
// output run should be 0 if this is just the first pass, 1 if final passs
testingReturnType processLine(char *lineInput)
{
	testingReturnType tempThingy;
	if ((lineInput[0]) == '\t') // should be instruction or data
	{
		if (type == -1)
		{
			throwError("ERROR! DATA/INSTRUCTION TYPE UNDEFINED!");
		}
		if (type == 1) // .code mode - process as instruction
		{
			processInstructions(&tempThingy, lineInput);
		}
		else if (type == 0) // .data mode - process as 64 bit data value
		{
			tempThingy.type = 0;
			char *valuePart = lineInput + 1; // skip the tab
			stripChars(valuePart, ' ');
			stripChars(valuePart, '\n');

			long long value = strtoll(valuePart, NULL, 0);

			sprintf(data + strlen(data), "\n\t%lld", value);

			incrementBytes(type); // add 8 bytes for data
			debuggingLineCount++;
		}
	}
	else if ((lineInput[0]) == ':') // label
	{
		tempThingy.type = 2;
		tempThingy.line = "";
		PD *newPD = malloc(sizeof(PD));
		newPD->replacementInt = memNumBytes;
		stripChars(lineInput, ' ');
		stripChars(lineInput, '\t');
		stripChars(lineInput, '\n');

		newPD->thingToReplace = strdup(lineInput);
		if (numRuns == 0)
			addPD(&listOfLabels, newPD);
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
		int newType = -1;
		if (strncmp(lineInput, ".data", 5) == 0)
		{
			newType = 0;
		}
		else if (strncmp(lineInput, ".code", 5) == 0)
		{
			newType = 1;
		}
		else
		{
			throwError("ERROR! INVALID LINE STARTING WITH DOT!");
		}

		// add new stuff only if new type so subsequent stuff gets combined
		if (type != -1 && type != newType)
		{
			if (type == 1)
			{
				if (isFirstLineInOutput)
				{
					strcat(codeAndDataCombined, ".code");
					isFirstLineInOutput = 0;
				}
				else
				{
					strcat(codeAndDataCombined, "\n.code");
				}
				strcat(codeAndDataCombined, code);
				code[0] = '\0';
			}
			else if (type == 0)
			{
				if (isFirstLineInOutput)
				{
					strcat(codeAndDataCombined, ".data");
					isFirstLineInOutput = 0;
				}
				else
				{
					strcat(codeAndDataCombined, "\n.data");
				}
				strcat(codeAndDataCombined, data);
				data[0] = '\0';
			}
		}

		tempThingy.type = newType;
		type = newType;
		if (newType == 1)
		{
			hasSeenAtLeast1Code = 1;
		}
	}
	else if (lineInput[0] == '\n')
	{
		return tempThingy;
	}
	else
	{
		throwError("ERROR! UNRECOGNIZED LINE FORMAT!");
	}

	return tempThingy;
}

void throwError(char *message)
{
	fprintf(stderr, "%s\n %i", message, debuggingLineCount);
	exit(EXIT_FAILURE);
}

void processInstructions(testingReturnType *input, char *lineInput)
{
	char *threeByThrees[] = {"add", "sub", "mul", "div", "xor"};
	char *fourByTwo[] = {"addi", "subi", "brnz"};
	if (startsWith2(lineInput, threeByThrees, 5))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
	}
	else if (startsWith2(lineInput, fourByTwo, 3))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 2);
		autoAdd(lineInput);

		// rd <- rd + L
	}
	else if (startsWith("call", lineInput))
	{

		handleValidateNumArgs(lineInput + 4 + 1, 1);
		// pc <- rd, saves return addr
		autoAdd(lineInput);
	}

	else if (startsWith("or", lineInput))
	{

		handleValidateNumArgs(lineInput + 2 + 1, 3);
		// rd <- rs | rt
		autoAdd(lineInput);
	}
	else if (startsWith("shftli", lineInput))
	{

		handleValidateNumArgs(lineInput + 6 + 1, 2);
		autoAdd(lineInput);
		// rd <- rd << L
	}
	else if (startsWith("shftl", lineInput))
	{

		handleValidateNumArgs(lineInput + 5 + 1, 3);
		autoAdd(lineInput);
		// rd <- rs << rt
	}
	else if (startsWith("shftri", lineInput))
	{

		handleValidateNumArgs(lineInput + 6 + 1, 2);
		autoAdd(lineInput);
		// rd <- rd >> L
	}
	else if (startsWith("shftr", lineInput))
	{
		handleValidateNumArgs(lineInput + 5 + 1, 3);
		autoAdd(lineInput);
		// rd <- rs >> rt
	}
	else if (startsWith("mov", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		autoAdd(lineInput);
		// Could be:
		// 1. mov rd, rs (Reg-to-Reg)
		// 2. mov rd, (rs)(L) (Load from memory)
		// 3. mov (rd)(L), rs (Store to memory)
	}
	else if (startsWith("brr", lineInput))
	{

		handleValidateNumArgs(lineInput + 3 + 1, 1);
		autoAdd(lineInput);
		// NOTE: can be brr r_d OR brr L
		//  Branch Relative: PC <- PC + L
	}
	else if (startsWith("brgt", lineInput))
	{

		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
		// pc <- rd if rs > rt
	}
	else if (startsWith("br", lineInput))
	{

		handleValidateNumArgs(lineInput + 2 + 1, 1);
		autoAdd(lineInput);
		// pc <- rd
	}
	else if (startsWith("return", lineInput))
	{
		handleValidateNumArgs(lineInput + 6 + 1, 0);

		autoAdd(lineInput);

		// No args
	}
	else if (startsWith("addf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
	}
	else if (startsWith("subf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
	}
	else if (startsWith("mulf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
	}
	else if (startsWith("divf", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 3);
		autoAdd(lineInput);
	}
	else if (startsWith("and", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 3);
		autoAdd(lineInput);
		// rd <- rs & rt
	}
	else if (startsWith("not", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		autoAdd(lineInput);
		// rd <- ~rs
	}
	else if (startsWith("priv", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 4);
		autoAdd(lineInput);
		// priv rd, rs, rt, L
	}
	// === MACROS (to be expanded later) ===
	else if (startsWith("clr", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 1);
		autoAddMacro(convertClrMacro(lineInput));
		// Macro: expands to xor rd, rd, rd
	}
	else if (startsWith("ld", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 2);
		autoAddMacro(convertLdMacro(lineInput));

		// Macro: expands to xor + addi/shftli chain
	}
	else if (startsWith("halt", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 0);
		autoAddMacro(convertHaltMacro(lineInput));

		// Macro: no args, expands to priv r0, r0, r0, 0x0
	}
	else if (startsWith("in", lineInput))
	{
		handleValidateNumArgs(lineInput + 2 + 1, 2);
		autoAddMacro(convertInMacro(lineInput));

		// Macro: expands to priv rd, rs, r0, 0x3
	}
	else if (startsWith("out", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 2);
		autoAddMacro(convertOutMacro(lineInput));

		// Macro: expands to priv rd, rs, r0, 0x4
	}
	else if (startsWith("push", lineInput))
	{
		handleValidateNumArgs(lineInput + 4 + 1, 1);
		autoAddMacro(convertPushMacro(lineInput));

		// Macro: push rd onto stack
	}
	else if (startsWith("pop", lineInput))
	{
		handleValidateNumArgs(lineInput + 3 + 1, 1);
		autoAddMacro(convertPopMacro(lineInput));

		// Macro: pop into rd from stack
	}
	else
	{
		throwError("ERROR! INVALID INSTRUCTION!");
	}
}

// todo: implement
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
	char *copy = malloc(sizeof(char) * (strlen(inputs) + 1));
	strcpy(copy, inputs);

	char *token = strtok(copy, delimiters);

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
	debuggingLineCount++;
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
			return listOfLabels.data[i]->replacementInt;
		}
	}
	throwError("ERROR! LABEL NOT RECOGNIZED!");
}

// Test functions:
void runTests()
{
	printf("TESTING MODE ACTIVE!");
}

// halt
char *convertHaltMacro(char *input)
{
	return "\n\tpriv r0, r0, r0, 0x0";
}

// clr rd
char *convertClrMacro(char *input)
{
	char *buffer = malloc(sizeof(char) * 30); // more than enough space
	buffer[0] = '\0';
	input += 4; // to get rid of the tab + clr
	char *token = strtok(input, delimiters);
	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);
	return buffer;
}

// ld rd, L
char *convertLdMacro(char *input)
{
	// int isZeros = 1; //means we're just starting off, leading bits are zero so can be discarded
	char *buffer = malloc(sizeof(char) * 500);
	buffer[0] = '\0';
	input += 3; // to get rid of the tab + ld
	long long value = 0;

	char *token = strtok(input, delimiters); // the register
	char *token2 = strtok(NULL, delimiters); // the data -- either a label or a literal

	if (token2[0] == ':')
	{
		if (numRuns == 0)
		{ // first pass, so inconsequential
			value = 0;
		}
		else
		{
			value = evaluateLabel(token2);
		}
	}
	else
	{
		value = strtoll(token2, NULL, 10);
	}

	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 60) & 0xF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 48) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 36) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 24) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 12) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value & 0xFFF));

	// one less than actual (11 instead of 12) since the data validation will add one //TODO: fix if we update the structure
	if (type == 0)
	{
		memNumBytes += 11 * 8;
	}
	else if (type == 1)
	{
		memNumBytes += 11 * 4;
	}
	else
	{
		throwError("ERROR! Instruction called before .code or .data");
	}

	return buffer;
}

// in rd, rs
char *convertInMacro(char *input)
{
	input += 3;								  // to get rid of the tab + in
	char *buffer = malloc(sizeof(char) * 40); // more than enough space
	buffer[0] = '\0';

	char *token = strtok(input, delimiters); // rd
	char *token2 = strtok(NULL, delimiters); // rs

	sprintf(buffer, "\n\tpriv %s, %s, r0, %i", token, token2, 0x3);
	return buffer;
}

// out rd, rs
char *convertOutMacro(char *input)
{
	input += 4;								  // to get rid of the tab + out
	char *buffer = malloc(sizeof(char) * 40); // more than enough space
	buffer[0] = '\0';

	char *token = strtok(input, delimiters); // rd
	char *token2 = strtok(NULL, delimiters); // rs

	sprintf(buffer, "\n\tpriv %s, %s, r0, %i", token, token2, 0x4);
	return buffer;
}

// push rd
char *convertPushMacro(char *input)
{
	input += 5;								  // to get rid of the tab + push
	char *buffer = malloc(sizeof(char) * 45); // more than enough space
	buffer[0] = '\0';
	char *token = strtok(input, delimiters); // rd

	sprintf(buffer + strlen(buffer), "\n\tmov (r31)(-8), %s", token);
	sprintf(buffer + strlen(buffer), "\n\tsubi r31, 8");

	// NOTE: only adding 1 extra instruction since other is counted by the handleValidateNumArgs //todo: fix if change
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

	return buffer;
}

// pop rd
char *convertPopMacro(char *input)
{
	input += 4;								  // to get rid of the tab + pop
	char *buffer = malloc(sizeof(char) * 45); // more than enough space
	buffer[0] = '\0';
	char *token = strtok(input, delimiters); // rd

	sprintf(buffer + strlen(buffer), "\n\tmov %s, (r31)(0)", token);
	sprintf(buffer + strlen(buffer), "\n\taddi r31, 8");

	// NOTE: only adding 1 extra instruction since other is counted by the handleValidateNumArgs //todo: fix if change
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

	return buffer;
}

char *parseAndFormatArgs(char *inputString)
{
	char *outputLine = malloc(sizeof(char) * 50); // once again, plenty of space
	outputLine[0] = '\0';
	char delims[] = " ,\t\n\r";
	char *token = strtok(inputString, delims);
	sprintf(outputLine + strlen(outputLine), "\n\t%s", token); // should be the first one -- the instruction
	token = strtok(NULL, delims);
	int isFirst = 1;
	while (1)
	{
		if (token != NULL)
		{
			if (isFirst)
			{
				isFirst = 0;
				sprintf(outputLine + strlen(outputLine), " %s", token);
			}
			else
			{
				sprintf(outputLine + strlen(outputLine), ", %s", token);
			}
		}
		else
		{
			break;
		}
		token = strtok(NULL, delims);
	}
	// printf("Here's what I'm returning: %s\n", outputLine);
	return outputLine;
}

void autoAdd(char *inputString)
{
	// 0 for data mode, 1 for int, -1 for unset
	if (type == 0)
	{
		strcat(data, parseAndFormatArgs(inputString));
	}
	else if (type == 1)
	{
		strcat(code, parseAndFormatArgs(inputString));
	}
	else
	{
		throwError("ERROR! Once again called before type set somehow...");
	}
}

void autoAddMacro(char *inputString)
{
	if (type == 0)
	{
		strcat(data, inputString);
	}
	else if (type == 1)
	{
		strcat(code, inputString);
	}
	else
	{
		throwError("ERROR! Once again called before type set somehow...");
	}
}