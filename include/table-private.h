/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H

#include "table.h"

struct table_t
{
    struct list_t **list;
    int size;
};

int hash(char* key, int size);

#endif
