#include <stdlib.h>
#include <stdio.h>
#include "ArrayList.h"

void newPDArrayList(PDArrayList *list)
{
    list->capacity = 4;
    list->size = 0;
    list->data = malloc(sizeof(PD) * list->capacity);
}

void addPD(PDArrayList *list, PD *item)
{
    if (list->size == list->capacity)
    {
        list->capacity = list->capacity * 2;
        list->data = realloc(list->data, list->capacity * sizeof(PD *));
    }
    list->data[list->size] = item;
    list->size = list->size + 1;
}
