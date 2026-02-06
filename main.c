#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArrayList.h"
#include <errno.h>
#include <stdint.h>

// yeah yeah ik I should put this in a seperate file, but I'll do that later if I have time
typedef struct StringBuilder
{
	char *data;
	size_t size;
	size_t capacity;
} StringBuilder;

void newStringBuilder(StringBuilder *sb)
{
	sb->capacity = 5000;
	sb->size = 0;
	sb->data = malloc(sb->capacity);
	sb->data[0] = '\0';
}

void addString(StringBuilder *sb, const char *str)
{
	size_t addsize = strlen(str);
	if (sb->size + addsize + 1 > sb->capacity)
	{
		while (sb->size + addsize + 1 > sb->capacity)
			sb->capacity *= 2;
		sb->data = realloc(sb->data, sb->capacity);
	}
	strcpy(sb->data + sb->size, str);
	sb->size += addsize;
}

void sbClear(StringBuilder *sb)
{
	sb->size = 0;
	sb->data[0] = '\0';
}

// Make sure registers start with R, labels start with L -- to pass invalid test cases -- spaces aren't allowed inside the label

#define IS_CURRENTLY_TESTING 0

#define FMT_RRR 0	 // rd, rs, rt
#define FMT_RR 1	 // rd, rs
#define FMT_RL 2	 // rd, L
#define FMT_R 3		 // rd
#define FMT_L 4		 // L
#define FMT_NONE 5	 // no params
#define FMT_RRRL 6	 // rd, rs, rt, L
#define FMT_MOV 7	 // special: 2 args but depends on mov
#define FMT_R_OR_L 8 // 1 arg: reg or lit for brr
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
void validateArgs(char *lineInput, int formatType);
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
void pushOtherInIfExists(int targetType);

// new stuff
void processIntermediateIntoBinary(char *intermediateFilePath, char *outputBinaryFilePath);
uint8_t getOpcode(char *line);
uint32_t processIntermediateLine(char *line);
uint32_t processOpcodeIntoFinalForm(uint8_t opcode, char *line);
uint32_t convertR(uint8_t opcode, uint8_t reg);
uint32_t convertRR(uint8_t opcode, uint8_t rd, uint8_t rs);
uint32_t convertRL(uint8_t opcode, uint8_t rd, int32_t L);
uint32_t convertRRR(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt);
uint32_t convertL(uint8_t opcode, int32_t L);
uint32_t convertNone(uint8_t opcode);
uint32_t convertRRRL(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt, int32_t L);
uint32_t convertMOV(uint8_t opcode, char *line);
uint8_t parseReg(char *registry);
uint64_t parse64BitNums(char *input);

PDArrayList listOfLabels;
const char delimiters[] = ", \t\n\r";
StringBuilder code;
StringBuilder data;
StringBuilder codeAndDataCombined;

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
	newStringBuilder(&code);
	newStringBuilder(&data);
	newStringBuilder(&codeAndDataCombined);

	if (!IS_CURRENTLY_TESTING)
	{
		processInputFile(inputFilePath);
		FILE *fptr;
		fptr = fopen(intermediateOutputFilePath, "w");
		fprintf(fptr, "%s", codeAndDataCombined.data);
		fclose(fptr);
		printf("%s", codeAndDataCombined.data);
		processIntermediateIntoBinary(intermediateOutputFilePath, binaryOutputFilePath);
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
			processLine(line);
		}

		numRuns++;
		sbClear(&code);
		sbClear(&data);
		sbClear(&codeAndDataCombined);
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
		if (code.size > 0)
		{
			if (isFirstLineInOutput)
			{
				addString(&codeAndDataCombined, ".code");
				isFirstLineInOutput = 0;
			}
			else
			{
				addString(&codeAndDataCombined, "\n.code");
			}
			addString(&codeAndDataCombined, code.data);
		}
		if (data.size > 0)
		{
			if (isFirstLineInOutput)
			{
				addString(&codeAndDataCombined, ".data");
				isFirstLineInOutput = 0;
			}
			else
			{
				addString(&codeAndDataCombined, "\n.data");
			}
			addString(&codeAndDataCombined, data.data);
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
			char *valuePart = lineInput + 1; // skip the tab with extra plus 1
			stripChars(valuePart, ' ');
			stripChars(valuePart, '\n');

			errno = 0;
			char *end;
			unsigned long long value = strtoull(valuePart, &end, 0);

			if (end == valuePart)
			{
				throwError("ERROR! INVALID DATA PASSED - not a long");
			}

			if (errno == ERANGE)
			{
				throwError("ERROR! Data value exceeds 64-bit range");
			}

			// pushOtherInIfExists(0); // push all the code in if needed before adding more data

			char temp[64];
			sprintf(temp, "\n\t%llu", value);
			addString(&data, temp);

			incrementBytes(type); // add 8 bytes for data
			debuggingLineCount++;
		}
	}
	else if ((lineInput[0]) == ':') // label found
	{
		if(lineInput[1] != 'L'){
			throwError("ERROR! INVALID LABEL FORMAT!");
		}
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
					addString(&codeAndDataCombined, ".code");
					isFirstLineInOutput = 0;
				}
				else
				{
					addString(&codeAndDataCombined, "\n.code");
				}
				addString(&codeAndDataCombined, code.data);
				sbClear(&code);
			}
			else if (type == 0)
			{
				if (isFirstLineInOutput)
				{
					addString(&codeAndDataCombined, ".data");
					isFirstLineInOutput = 0;
				}
				else
				{
					addString(&codeAndDataCombined, "\n.data");
				}
				addString(&codeAndDataCombined, data.data);
				sbClear(&data);
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
	if (startsWith("shftri", lineInput) || startsWith("shftli", lineInput) || startsWith("addi", lineInput) || startsWith("subi", lineInput))
	{
		validateArgs(lineInput, FMT_RL);
		autoAdd(lineInput);
	}
	else if (startsWith("brnz", lineInput) || startsWith("not", lineInput))
	{
		validateArgs(lineInput, FMT_RR);
		autoAdd(lineInput);
	}
	else if (startsWith("brr", lineInput))
	{
		validateArgs(lineInput, FMT_R_OR_L);
		autoAdd(lineInput);
	}
	else if (startsWith("priv", lineInput))
	{
		validateArgs(lineInput, FMT_RRRL);
		autoAdd(lineInput);
	}
	else if (startsWith("add", lineInput) || startsWith("sub", lineInput) ||
			 startsWith("mul", lineInput) || startsWith("div", lineInput) ||
			 startsWith("and", lineInput) || startsWith("xor", lineInput) || startsWith("or", lineInput) || startsWith("addf", lineInput) || startsWith("subf", lineInput) ||
			 startsWith("mulf", lineInput) || startsWith("divf", lineInput) ||
			 startsWith("brgt", lineInput) || startsWith("shftr", lineInput) || startsWith("shftl", lineInput))
	{
		validateArgs(lineInput, FMT_RRR);
		autoAdd(lineInput);
	}
	else if (startsWith("call", lineInput) || startsWith("br", lineInput))
	{
		validateArgs(lineInput, FMT_R);
		autoAdd(lineInput);
	}

	else if (startsWith("mov", lineInput))
	{
		validateArgs(lineInput, FMT_MOV);
		autoAdd(lineInput);
	}
	else if (startsWith("return", lineInput))
	{
		validateArgs(lineInput, FMT_NONE);
		autoAdd(lineInput);
	}
	// macro expansion stuff
	else if (startsWith("halt", lineInput))
	{
		validateArgs(lineInput, FMT_NONE);
		autoAddMacro(convertHaltMacro(lineInput));
	}
	else if (startsWith("push", lineInput))
	{
		validateArgs(lineInput, FMT_R);
		autoAddMacro(convertPushMacro(lineInput));
	}
	else if (startsWith("pop", lineInput))
	{
		validateArgs(lineInput, FMT_R);
		autoAddMacro(convertPopMacro(lineInput));
	}
	else if (startsWith("clr", lineInput))
	{
		validateArgs(lineInput, FMT_R);
		autoAddMacro(convertClrMacro(lineInput));
	}
	else if (startsWith("out", lineInput))
	{
		validateArgs(lineInput, FMT_RR);
		autoAddMacro(convertOutMacro(lineInput));
	}
	else if (startsWith("ld", lineInput))
	{
		validateArgs(lineInput, FMT_RL);
		autoAddMacro(convertLdMacro(lineInput));
	}
	else if (startsWith("in", lineInput))
	{
		validateArgs(lineInput, FMT_RR);
		autoAddMacro(convertInMacro(lineInput));
	}
	else
	{
		printf("%s", lineInput);
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
	// ERMmmmm -- this is kinda sketchy bc it could trigger weird stuff if you get add i0 --> addi0 which passes but is wrong...
	if (strncmp(searchString, string + 1, strlen(searchString)))
	{
		// failed basic check
		char *copy = malloc(sizeof(char) * (strlen(string) + 1)); // so I dont brick the original
		strcpy(copy, string);

		stripChars(copy, ' ');
		stripChars(copy, '\t');
		return !strncmp(searchString, copy, strlen(searchString));
	}
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

// Returns expected arg count for a format type
int getExpectedArgCount(int formatType)
{
	switch (formatType)
	{
	case FMT_RRR:
		return 3; // rd, rs, rt
	case FMT_RR:
		return 2; // rd, rs
	case FMT_RL:
		return 2; // rd, L
	case FMT_R:
		return 1; // rd
	case FMT_L:
		return 1; // L
	case FMT_NONE:
		return 0; // (none)
	case FMT_RRRL:
		return 4; // rd, rs, rt, L
	case FMT_MOV:
		return 2; // 2 args (various formats)
	case FMT_R_OR_L:
		return 1; // 1 arg (register or literal)
	default:
		return -1;
	}
}

// checks line against format, inputs whole line and strips instruction + tab
void validateArgs(char *lineInput, int formatType)
{
	char *copy = malloc(sizeof(char) * (strlen(lineInput) + 1));
	strcpy(copy, lineInput);

	char *token = strtok(copy, delimiters);

	int count = 0;
	char *args[5];
	while ((token = strtok(NULL, delimiters)) != NULL && count < 5)
	{
		args[count] = token;
		count++;
	}

	int expected = getExpectedArgCount(formatType);
	if (count != expected)
	{
		printf("original: %s, count is %i, expected is %i\n", lineInput, count, expected);
		printf("format: %i, count is %i, expected is %i\n", formatType, count, getExpectedArgCount(formatType));
		throwError("ERROR! INVALID SYNTAX FOR INSTRUCTION PASSED!");
	}

	if (formatType == FMT_RRR)
	{
		parseReg(args[0]);
		parseReg(args[1]);
		parseReg(args[2]);
	}
	else if (formatType == FMT_RR)
	{
		parseReg(args[0]);
		parseReg(args[1]);
	}
	else if (formatType == FMT_RL)
	{
		parseReg(args[0]);
	}
	else if (formatType == FMT_R)
	{
		parseReg(args[0]);
	}
	else if (formatType == FMT_RRRL)
	{
		parseReg(args[0]);
		parseReg(args[1]);
		parseReg(args[2]);
	}
	else if (formatType == FMT_MOV)
	{
		if (args[0][0] == '(')
		{
			char *temp = args[0] + 1;
			char *end = strchr(temp, ')'); //replace the ending paren with null to cut string
			if (end) *end = '\0';
			parseReg(temp);
		}
		else
		{
			parseReg(args[0]);
		}
	}
	else if (formatType == FMT_R_OR_L)
	{
		if (args[0][0] == 'r' || args[0][0] == 'R')
		{
			parseReg(args[0]);
		}
	}

	free(copy);
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
	return "\n\tpriv r0, r0, r0, 0";
}

// clr rd
char *convertClrMacro(char *input)
{
	char *buffer = malloc(sizeof(char) * 30);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 4;
	char *token = strtok(copy, delimiters);
	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);
	return buffer;
}

// ld rd, L
char *convertLdMacro(char *input)
{
	char *buffer = malloc(sizeof(char) * 500);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 3;
	long long value = 0;

	char *token = strtok(copy, delimiters);
	char *token2 = strtok(NULL, delimiters);

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
		value = strtoll(token2, NULL, 0);
	}

	//ze brick lol
	sprintf(buffer, "\n\txor %s, %s, %s", token, token, token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 52) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 40) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 28) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 16) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 12", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, (value >> 4) & 0xFFF);
	sprintf(buffer + strlen(buffer), "\n\tshftli %s, 4", token);
	sprintf(buffer + strlen(buffer), "\n\taddi %s, %lld", token, value & 0xF);

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
	char *buffer = malloc(sizeof(char) * 40);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 3;

	char *token = strtok(copy, delimiters);
	char *token2 = strtok(NULL, delimiters);

	sprintf(buffer, "\n\tpriv %s, %s, r0, %i", token, token2, 0x3);
	return buffer;
}

// out rd, rs
char *convertOutMacro(char *input)
{
	char *buffer = malloc(sizeof(char) * 40);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 4;

	char *token = strtok(copy, delimiters);
	char *token2 = strtok(NULL, delimiters);

	sprintf(buffer, "\n\tpriv %s, %s, r0, %i", token, token2, 0x4);
	return buffer;
}

// push rd
char *convertPushMacro(char *input)
{
	char *buffer = malloc(sizeof(char) * 45);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 5;
	char *token = strtok(copy, delimiters);

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
	char *buffer = malloc(sizeof(char) * 45);
	buffer[0] = '\0';
	char *copy = malloc(sizeof(char) * (strlen(input) + 1));
	strcpy(copy, input);
	copy += 4;
	char *token = strtok(copy, delimiters);

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
	char *outputLine = malloc(sizeof(char) * 70); // should be more than enough and I'm lowk too lazy to do malloc
	outputLine[0] = '\0';
	char delims[] = " ,\t\n\r";
	char *copy = malloc(sizeof(char) * (strlen(inputString) + 1));
	strcpy(copy, inputString);
	char *token = strtok(copy, delims);
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

// make sure all data from other one is pushed in iff it exists
void pushOtherInIfExists(int targetType)
{
	if (targetType == 1 && data.size > 0)
	{
		if (isFirstLineInOutput)
		{
			addString(&codeAndDataCombined, ".data");
			isFirstLineInOutput = 0;
		}
		else
		{
			addString(&codeAndDataCombined, "\n.data");
		}
		addString(&codeAndDataCombined, data.data);
		sbClear(&data);
	}
	else if (targetType == 0 && code.size > 0)
	{
		if (isFirstLineInOutput)
		{
			addString(&codeAndDataCombined, ".code");
			isFirstLineInOutput = 0;
		}
		else
		{
			addString(&codeAndDataCombined, "\n.code");
		}
		addString(&codeAndDataCombined, code.data);
		sbClear(&code);
	}
}

void autoAdd(char *inputString)
{
	// 0 for data mode, 1 for int, -1 for unset
	if (type == 0)
	{
		pushOtherInIfExists(0);
		addString(&data, parseAndFormatArgs(inputString));
	}
	else if (type == 1)
	{
		pushOtherInIfExists(1);
		addString(&code, parseAndFormatArgs(inputString));
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
		pushOtherInIfExists(0);
		addString(&data, inputString);
	}
	else if (type == 1)
	{
		pushOtherInIfExists(1);
		addString(&code, inputString);
	}
	else
	{
		throwError("ERROR! Once again called before type set somehow...");
	}
}

/// Pretend this is a new file without all the other stuff for cleanliness
//-1 for unset, 0 for data, 1 for code
int currentType = -1;

void processIntermediateIntoBinary(char *intermediateFilePath, char *outputBinaryFilePath)
{
	FILE *inputFile;
	FILE *outputFile = fopen(outputBinaryFilePath, "wb");
	if ((inputFile = fopen(intermediateFilePath, "r")) != NULL)
	{
		char line[1024];
		int BUFFER_SIZE = 1024;
		while (fgets(line, BUFFER_SIZE, inputFile) != NULL)
		{
			if (strncmp(line, ".code", 5) == 0)
			{
				currentType = 1;
				continue;
			}
			else if (strncmp(line, ".data", 5) == 0)
			{
				currentType = 0;
				continue;
			}

			if (currentType == 0)
			{
				uint64_t dataVal = parse64BitNums(line);
				fwrite(&dataVal, sizeof(uint64_t), 1, outputFile);
			}
			else if (currentType == 1)
			{
				uint32_t result = processIntermediateLine(line);
				fwrite(&result, sizeof(uint32_t), 1, outputFile);
			}
		}
		fclose(inputFile);
	}
	else
	{
		throwError("ERROR! NO INTERMEDIATE FOUND!");
	}
	fclose(outputFile);
}
uint8_t getOpcode(char *line)
{
	char *copy = malloc(sizeof(char) * (strlen(line) + 1));
	strcpy(copy, line);
	char *token = strtok(copy, delimiters); // should be instruction

	if (strcmp(token, "and") == 0)
		return 0x0;
	if (strcmp(token, "or") == 0)
		return 0x1;
	if (strcmp(token, "xor") == 0)
		return 0x2;
	if (strcmp(token, "not") == 0)
		return 0x3;
	if (strcmp(token, "shftr") == 0)
		return 0x4;
	if (strcmp(token, "shftri") == 0)
		return 0x5;
	if (strcmp(token, "shftl") == 0)
		return 0x6;
	if (strcmp(token, "shftli") == 0)
		return 0x7;
	if (strcmp(token, "br") == 0)
		return 0x8;
	if (strcmp(token, "brr") == 0)
	{
		token = strtok(NULL, delimiters);
		if (token[0] == 'r' || token[0] == 'R')
		{
			return 0x9;
		}
		else
		{
			return 0xa;
		}
	}
	if (strcmp(token, "brnz") == 0)
		return 0xb;
	if (strcmp(token, "call") == 0)
		return 0xc;
	if (strcmp(token, "return") == 0)
		return 0xd;
	if (strcmp(token, "brgt") == 0)
		return 0xe;
	if (strcmp(token, "priv") == 0)
		return 0xf;
	if (strcmp(token, "mov") == 0)
	{
		token = strtok(NULL, delimiters);
		if (token[0] == '(')
		{
			return 0x13;
		}
		else if (token[0] == 'r')
		{
			token = strtok(NULL, delimiters);
			if (token[0] == '(')
			{
				return 0x10;
			}
			else if (token[0] == 'r')
			{
				return 0x11;
			}
			else
			{
				return 0x12;
			}
		}
		throwError("Unable to parse MOV!!!!");
	}
	if (strcmp(token, "addf") == 0)
		return 0x14;
	if (strcmp(token, "subf") == 0)
		return 0x15;
	if (strcmp(token, "mulf") == 0)
		return 0x16;
	if (strcmp(token, "divf") == 0)
		return 0x17;
	if (strcmp(token, "add") == 0)
		return 0x18;
	if (strcmp(token, "addi") == 0)
		return 0x19;
	if (strcmp(token, "sub") == 0)
		return 0x1a;
	if (strcmp(token, "subi") == 0)
		return 0x1b;
	if (strcmp(token, "mul") == 0)
		return 0x1c;
	if (strcmp(token, "div") == 0)
		return 0x1d;
	if (strcmp(token, ".code") == 0 || strcmp(token, ".data") == 0)
		return 0;
	throwError("ERROR PARSING OPCODE!");
}

uint32_t processOpcodeIntoFinalForm(uint8_t opcode, char *line)
{
	// a bunch of if statements based on format
}

uint32_t processIntermediateLine(char *line)
{
	char *lineCopy = malloc(strlen(line) + 1);
	strcpy(lineCopy, line);
	uint8_t opcode = getOpcode(line);
	char *token = strtok(lineCopy, delimiters);

	if (opcode == 0x0 || opcode == 0x1 || opcode == 0x2 || opcode == 0x4 ||
		opcode == 0x6 || opcode == 0xe || opcode == 0x14 ||
		opcode == 0x15 || opcode == 0x16 || opcode == 0x17 || opcode == 0x18 ||
		opcode == 0x1a || opcode == 0x1c || opcode == 0x1d)
	{
		uint8_t rd = parseReg(strtok(NULL, delimiters));
		uint8_t rs = parseReg(strtok(NULL, delimiters));
		uint8_t rt = parseReg(strtok(NULL, delimiters));
		return convertRRR(opcode, rd, rs, rt);
	}
	else if (opcode == 0x5 || opcode == 0x7 || opcode == 0x19 || opcode == 0x1b)
	{
		uint8_t rd = parseReg(strtok(NULL, delimiters));
		int32_t L = (int32_t)strtol(strtok(NULL, delimiters), NULL, 0);
		return convertRL(opcode, rd, L);
	}
	else if (opcode == 0x3 || opcode == 0xb)
	{
		uint8_t rd = parseReg(strtok(NULL, delimiters));
		uint8_t rs = parseReg(strtok(NULL, delimiters));
		return convertRR(opcode, rd, rs);
	}
	else if (opcode == 0x8 || opcode == 0x9 || opcode == 0xc)
	{
		uint8_t rd = parseReg(strtok(NULL, delimiters));
		return convertR(opcode, rd);
	}
	else if (opcode == 0xa)
	{
		int32_t L = (int32_t)strtol(strtok(NULL, delimiters), NULL, 0);
		return convertL(opcode, L);
	}
	else if (opcode == 0xd)
	{
		// for return i think
		return convertNone(opcode);
	}
	else if (opcode == 0xf) // priv
	{
		uint8_t rd = parseReg(strtok(NULL, delimiters));
		uint8_t rs = parseReg(strtok(NULL, delimiters));
		uint8_t rt = parseReg(strtok(NULL, delimiters));
		int32_t L = (int32_t)strtol(strtok(NULL, delimiters), NULL, 0);
		return convertRRRL(opcode, rd, rs, rt, L);
	}
	else if (opcode == 0x10 || opcode == 0x11 || opcode == 0x12 || opcode == 0x13)
	{
		return convertMOV(opcode, line);
	}
}

uint32_t convertR(uint8_t opcode, uint8_t reg)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += ((reg & 0x1F) << 22);
	return temp;
}

uint32_t convertRR(uint8_t opcode, uint8_t rd, uint8_t rs)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += ((rd & 0x1F) << 22);
	temp += ((rs & 0x1F) << 17);
	return temp;
}

uint32_t convertRL(uint8_t opcode, uint8_t rd, int32_t L)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += ((rd & 0x1F) << 22);
	temp += (L & 0xFFF);
	return temp;
}

uint32_t convertRRR(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += ((rd & 0x1F) << 22);
	temp += ((rs & 0x1F) << 17);
	temp += ((rt & 0x1F) << 12);
	return temp;
}

uint32_t convertL(uint8_t opcode, int32_t L)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += (L & 0xFFF);
	return temp;
}

uint32_t convertNone(uint8_t opcode)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	return temp;
}

uint32_t convertRRRL(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t rt, int32_t L)
{
	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;
	temp += ((rd & 0x1F) << 22);
	temp += ((rs & 0x1F) << 17);
	temp += ((rt & 0x1F) << 12);
	temp += (L & 0xFFF);
	return temp;
}

// mov has 4 forms:
// 0x10: mov rd, (rs)(L)    - load from memory
// 0x11: mov rd, rs         - register to register
// 0x12: mov rd, L          - load literal (sets bits 52:63)
// 0x13: mov (rd)(L), rs    - store to memory

// TODO: fix to make sure compatable with all mov stuff
uint32_t convertMOV(uint8_t opcode, char *line)
{
	char *copy = malloc(sizeof(char) * (strlen(line) + 1));
	strcpy(copy, line);
	char *delims = " ,\t\n\r()";
	char *token = strtok(copy, delims);

	uint32_t temp = 0;
	temp += opcode;
	temp <<= 27;

	if (opcode == 0x10)
	{
		// mov rd, (rs)(L)
		uint8_t rd = parseReg(strtok(NULL, delims));
		uint8_t rs = parseReg(strtok(NULL, delims));
		int32_t L = (int32_t)strtol(strtok(NULL, delims), NULL, 0);
		temp += ((rd & 0x1F) << 22);
		temp += ((rs & 0x1F) << 17);
		temp += (L & 0xFFF);
	}
	else if (opcode == 0x11)
	{
		// mov rd, rs
		uint8_t rd = parseReg(strtok(NULL, delims));
		uint8_t rs = parseReg(strtok(NULL, delims));
		temp += ((rd & 0x1F) << 22);
		temp += ((rs & 0x1F) << 17);
	}
	else if (opcode == 0x12)
	{
		// mov rd, L
		uint8_t rd = parseReg(strtok(NULL, delims));
		int32_t L = (int32_t)strtol(strtok(NULL, delims), NULL, 0);
		temp += ((rd & 0x1F) << 22);
		temp += (L & 0xFFF);
	}
	else if (opcode == 0x13)
	{
		// mov (rd)(L), rs
		uint8_t rd = parseReg(strtok(NULL, delims));
		int32_t L = (int32_t)strtol(strtok(NULL, delims), NULL, 0);
		uint8_t rs = parseReg(strtok(NULL, delims));
		temp += ((rd & 0x1F) << 22);
		temp += ((rs & 0x1F) << 17);
		temp += (L & 0xFFF);
	}

	free(copy);
	return temp;
}

uint8_t parseReg(char *registry)
{
	if (registry[0] != 'r' && registry[0] != 'R')
	{

		throwError("ERROR! Register must start with 'r'!");
	}
	unsigned long regNum = strtoul(registry + 1, NULL, 0);
	if (regNum > 31)
	{
		throwError("ERROR! Register number must be between 0 and 31!");
	}
	return (uint8_t)regNum;
}

uint64_t parse64BitNums(char *input)
{
	// Theoretically I don't need any of these except for the tab strip, but whatevs
	stripChars(input, ' ');
	stripChars(input, ',');
	stripChars(input, '\t');
	stripChars(input, '\n');
	stripChars(input, '\r');

	errno = 0;
	char *end;
	uint64_t value = strtoull(input, &end, 0);

	// also theoretically don't need...
	if (end == input)
	{
		throwError("ERROR! Invalid 64-bit data value - not a number");
	}

	if (errno == ERANGE)
	{
		throwError("ERROR! 64-bit data value out of range");
	}

	return value;
}