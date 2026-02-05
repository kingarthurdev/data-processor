#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct testingReturnType
{
    char *line;
    int type; // 0 for data type, 1 for code type, 2 for label, -1 for neither
} testingReturnType;


//prior defs:
testingReturnType processLine(char *lineInput);
void processInputFile(char *inputFilePath);
void throwError(char *message);

// initialize some "string builders" and hope 5k chars is enough
char code[5000] = {0};
char data[5000] = {0};

//0 for data mode, 1 for int, -1 for unset
int type; 

//0 for false, 1 for true
int hasSeenAtLeast1Code = 0; 

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
    if ((lineInput[0]) == '\t')//should be instruction
    {
        if(type == -1){
            throwError("ERROR! DATA/INSTRUCTION TYPE UNDEFINED!");
        }

    }
    else if ((lineInput[0]) == ':')//label
    {
        tempThingy.type = 2;
        tempThingy.line = "";

        //todo: add to symbol table (like hash table) and then do like a preprocessor directive cleanup at the very end once all the things have been resolved. 
        //lowk too lazy to make hashtable 
    }
    else if ((lineInput[0]) == ';') //comment
    {
        tempThingy.type = -1;
        tempThingy.line = "";
    }
    else if ((lineInput[0]) =='.'){ //either .code or .data
        if(strncmp(lineInput, ".data", 5) == 0){
            tempThingy.type = 0;
            type = 0;

        }else if(strncmp(lineInput, ".code", 5) == 0){
            tempThingy.type = 1;
            type = 1;
            hasSeenAtLeast1Code = 1; 



        }else{
            throwError("ERROR! INVALID LINE STARTING WITH DOT!");
        }
    }else{
        throwError("ERROR! UNRECOGNIZED LINE FORMAT!");
    }


    return tempThingy;
}

void throwError(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}