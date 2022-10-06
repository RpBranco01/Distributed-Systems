/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table-private.h"
#include "list-private.h"
#include "string.h"


int hash(char* key, int size){
    int sum = 0;
    for (int i = 0; i <= strlen(key); i++){
        sum += key[i];
    }
    return (sum % size);
}