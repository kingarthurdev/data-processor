#ifndef PD_ARRAY_LIST_H
#define PD_ARRAY_LIST_H

#include <stdlib.h>
#include <stdio.h>

//pd stands for preprocessorDirective
typedef struct PD {
    char *thingToReplace;
    long long replacementInt;
} PD;

typedef struct PDArrayList {
    PD **data;
    int capacity;
    int size;
} PDArrayList;

void newPDArrayList(PDArrayList *list);
void addPD(PDArrayList *list, PD *item);

#endif