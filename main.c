#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArrayList.h"

// yeah yeah ik I should put this in a seperate file, but I'll do that later if I have time
typedef struct StringBuilder
{
	char *data;
	int size;
	int capacity;
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
	int addsize = strlen(str);
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
		printf("%s", codeAndDataCombined.data);
		FILE *fptr;
		fptr = fopen(intermediateOutputFilePath, "w");
		fprintf(fptr, "%s", codeAndDataCombined.data);
		fclose(fptr);

		printf("%s", codeAndDataCombined.data);
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
		if (type == 1 && code.size > 0)
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
		else if (type == 0 && data.size > 0)
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

		printf("final output: %s", codeAndDataCombined.data);
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

			char temp[64];
			sprintf(temp, "\n\t%lld", value);
			addString(&data, temp);

			incrementBytes(type); // add 8 bytes for data
			debuggingLineCount++;
		}
	}
	else if ((lineInput[0]) == ':') // label found
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
	char *copy = malloc(sizeof(char) * (strlen(lineInput) + 1)); // so I dont brick the original
	strcpy(copy, lineInput);

	char *token = strtok(copy, delimiters);

	int count = 0;
	while ((token = strtok(NULL, delimiters)) != NULL)
	{
		count++;
	}

	int expected = getExpectedArgCount(formatType);
	if (count != expected)
	{
		printf("original: %s, count is %i, expected is %i\n", lineInput, count, expected);
		printf("format: %i, count is %i, expected is %i\n", formatType, count, getExpectedArgCount(formatType));

		throwError("ERROR! INVALID SYNTAX FOR INSTRUCTION PASSED!");
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
	return "\n\tpriv r0, r0, r0, 0x0";
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
	char *outputLine = malloc(sizeof(char) * 256);
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

void autoAdd(char *inputString)
{
	// 0 for data mode, 1 for int, -1 for unset
	if (type == 0)
	{
		addString(&data, parseAndFormatArgs(inputString));
	}
	else if (type == 1)
	{
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
		addString(&data, inputString);
	}
	else if (type == 1)
	{
		addString(&code, inputString);
	}
	else
	{
		throwError("ERROR! Once again called before type set somehow...");
	}
}