#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct testingReturnType
{
    char *line;
    int type; // 0 for data type, 1 for code type, 2 for label, -1 for neither
} testingReturnType;

// prior defs:
testingReturnType processLine(char *lineInput);
void processInputFile(char *inputFilePath);
void throwError(char *message);
int validateNumArgs(char *inputs, int numExpectedArgs);
void stripChars(char *input, char charToStrip);
int startsWith(char *searchString, char *string);
void processInstructions(testingReturnType *input, char *lineInput);
void handleValidateNumArgs(char *inputs, int numExpectedArgs);

const char delimiters[] = ", ";
char *token;

// initialize some "string builders" and hope 5k chars is enough
char code[5000] = {0};
char data[5000] = {0};

// 0 for data mode, 1 for int, -1 for unset
int type;

// 0 for false, 1 for true
int hasSeenAtLeast1Code = 0;

// count # of prior instructions
int memNumBytes = 0;

int main(int argc, char *argv[])
{
    printf("fucking kill me bro\n");

    char *inputFilePath = argv[1];
    char *intermediateOutputFilePath = argv[2];
    char *binaryOutputFilePath = argv[3];

    processInputFile(inputFilePath);

    return 0;
}

void processInputFile(char *inputFilePath)
{
    FILE *file;
    if ((file = fopen(inputFilePath, "r")) != NULL)
    {
        int BUFFER_SIZE = 1024; // Accepting 1024 chars in case they feed in a decently long line
        char line[1024];
        while (fgets(line, BUFFER_SIZE, file) != NULL)
        {
            processLine(line);
        }
    }
    else
    {
        throwError("ERROR! INVALID FILE INPUT PASSED (cannot access file)!");
    }
}

testingReturnType processLine(char *lineInput)
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
    if (startsWith("add", lineInput))
    {
        handleValidateNumArgs(lineInput + 3, 3);
        // rd <- rs + rt (Opcode 0x18)
    }
    else if (startsWith("addi", lineInput))
    {
        handleValidateNumArgs(lineInput + 4, 2);
        // rd <- rd + L (Opcode 0x19)
    }
    else if (startsWith("sub", lineInput))
    {
        handleValidateNumArgs(lineInput + 3, 3);
        // rd <- rs - rt (Opcode 0x1a)
    }
    else if (startsWith("subi", lineInput))
    {
        handleValidateNumArgs(lineInput + 4, 2);
        // rd <- rd - L (Opcode 0x1b)
    }
    else if (startsWith("mul", lineInput))
    {
        handleValidateNumArgs(lineInput + 3, 3);
        // rd <- rs * rt (Opcode 0x1c)
    }
    else if (startsWith("div", lineInput))
    {
        handleValidateNumArgs(lineInput + 3, 3);
        // rd <- rs / rt (Opcode 0x1d)
    }
    else if (startsWith("and", lineInput))
    {
        // rd <- rs & rt (Opcode 0x1e)
    }
    else if (startsWith("or", lineInput))
    {
        // rd <- rs | rt (Opcode 0x1f)
    }
    else if (startsWith("xor", lineInput))
    {
        // rd <- rs ^ rt (Opcode 0x20)
    }
    else if (startsWith("shftl", lineInput))
    {
        // rd <- rs << rt (Opcode 0x21)
    }
    else if (startsWith("shftli", lineInput))
    {
        // rd <- rd << L (Opcode 0x22)
    }
    else if (startsWith("shftr", lineInput))
    {
        // rd <- rs >> rt (Opcode 0x23)
    }
    else if (startsWith("shftri", lineInput))
    {
        // rd <- rd >> L (Opcode 0x24)
    }
    else if (startsWith("mov", lineInput))
    {
        // Could be:
        // 1. mov rd, rs (Reg-to-Reg)
        // 2. mov rd, (rs)(L) (Load from memory)
        // 3. mov (rd)(L), rs (Store to memory)
    }
    else if (startsWith("brr", lineInput))
    { // NOTE: can be brr r_d OR brr L
      //  Branch Relative: PC <- PC + L (Opcode 0x40)
    }
    else if (startsWith("br", lineInput))
    {
        // Branch Register: PC <- rd (Opcode 0x41)
    }
    else if (startsWith("call", lineInput))
    {
        // call rd (Opcode 0x42)
    }
    else if (startsWith("return", lineInput))
    {
        // return (Opcode 0x43)
    }
    else if (startsWith("trap", lineInput))
    {
        // System call (Opcode 0x01)
    }
    else if (startsWith("addf", lineInput))
    {
        // Opcode 0x14
    }
    else if (startsWith("subf", lineInput))
    {
        // Opcode 0x15
    }
    else if (startsWith("mulf", lineInput))
    {
        // Opcode 0x16
    }
    else if (startsWith("divf", lineInput))
    {
        // Opcode 0x17
    }
    else if (startsWith("clr", lineInput))
    {
        // Expand to: xor rd, rd, rd
    }
    else if (startsWith("ld", lineInput))
    {
        // Expand to: xor -> addi/shftli chain (as discussed)
    }
    else if (startsWith("halt", lineInput))
    {
        // Expand to: priv r0, r0, r0, 0
    }
    else if (startsWith("not", lineInput))
    {
        // Typically expands to: nand rd, rs, rs (or equivalent)
    }
    else if (startsWith("priv", lineInput))
    {
        // Typically expands to: nand rd, rs, rs (or equivalent)
    }
    else
    {
        throwError("ERROR! INVALID INSTRUCTION!");
    }
}

// Return 1 if true, 0 if false
int startsWith(char *searchString, char *string)
{
    return !strncmp(searchString, string, strlen(string));
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
    int count = 0;
    while (1)
    {
        token = strtok(inputs, delimiters);
        if (token != NULL)
        {
            count++;
        }
        else
        {
            break;
        }
    }
    return count == numExpectedArgs;
}

void handleValidateNumArgs(char *inputs, int numExpectedArgs)
{
    if (validateNumArgs == 0)
    {
        throwError("ERROR! INVALID SYNTAX FOR INSTRUCTION PASSED!");
    }
}