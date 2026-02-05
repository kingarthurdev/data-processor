#include <stdlib.h>
#include <stdio.h>

//pd stands for preprocessorDirective
typedef struct PD{
    char *thingToReplace;
    char *replacementString;
} PD;

typedef struct PD
{
    PD **data;
    int capacity;
    int size;
} PDArrayList;

void newPDArrayList(PDArrayList *list)
{
    list->capacity = 4;
    list->size = 0;
    list->data = malloc(sizeof(PD) * list->capacity);
}

void addNode(PDArrayList *list, PD *item)
{
    if (list->size == list->capacity)
    {
        list->capacity = list->capacity * 2;
        list->data = realloc(list->data, list->capacity * sizeof(PD *));
    }
    list->data[list->size] = item;
    list->size = list->size + 1;
}
