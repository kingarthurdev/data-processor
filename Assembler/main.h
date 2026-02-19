#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArrayList.h"
#include <stdint.h>

// yeah yeah ik I should put this in a seperate file, but I'll do that later if I have time
typedef struct StringBuilder
{
	char *data;
	size_t size;
	size_t capacity;
} StringBuilder;

void newStringBuilder(StringBuilder *sb);
void addString(StringBuilder *sb, const char *str);
void sbClear(StringBuilder *sb);

// Make sure registers start with R, labels start with L -- to pass invalid test cases -- spaces aren't allowed inside the label

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

testingReturnType processLine(char *lineInput);
void processInputFile(char *inputFilePath);
void throwError(char *message);
void processInstructions(testingReturnType *input, char *lineInput);
int startsWith(char *searchString, char *string);
int startsWith2(char *inputString, char *searchStrings[], int numStrings);
void stripChars(char *input, char charToStrip);
void validateArgs(char *lineInput, int formatType);
void validateLiteral(long long value, int isUnsigned);
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
int getExpectedArgCount(int formatType);
long long evaluateLabel(char *label);

extern char *globalOutputIntermediate;
extern char *globalOutputBinary;
extern PDArrayList listOfLabels;
extern const char delimiters[];
extern StringBuilder code;
extern StringBuilder data;
extern StringBuilder codeAndDataCombined;
extern int type;
extern int hasSeenAtLeast1Code;
extern long long memNumBytes;
extern int numRuns;
extern int debuggingLineCount;
extern int isFirstLineInOutput;

#endif
