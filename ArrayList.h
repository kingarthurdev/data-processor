#ifndef PD_ARRAY_LIST_H
#define PD_ARRAY_LIST_H

#include <stdlib.h>
#include <stdio.h>

typedef struct PD {
    char *thingToReplace;
    char *replacementString;
} PD;

typedef struct {
    PD **data;
    int capacity;
    int size;
} PDArrayList;

void newPDArrayList(PDArrayList *list);
void addNode(PDArrayList *list, PD *item);

#endif