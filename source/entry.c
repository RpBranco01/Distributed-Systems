/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "entry.h"
#include "stdlib.h"
#include <string.h>

/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data)
{
    struct entry_t *entry;
    if (data == NULL || key == NULL)
    {
        return NULL;
    }
    entry = malloc(sizeof(struct entry_t));
    if (entry == NULL)
    {
        return NULL;
    }

    entry->value = data;
    entry->key = key;
    return entry;
}

/* Função que inicializa os elementos de uma entrada na tabela com o
 * valor NULL.
 */
void entry_initialize(struct entry_t *entry)
{
    if (entry != NULL)
    {
        entry->key = NULL;
        entry->value = NULL;
    }
}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry)
{
    if (entry != NULL)
    {
        if (entry->value != NULL)
        {
            data_destroy(entry->value);
        }
        if (entry->key != NULL)
        {
            free(entry->key);
        }
        free(entry);
    }
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry)
{
    struct entry_t *new_entry;
    struct data_t *data;
    char *key;
    if (entry == NULL)
    {
        return NULL;
    }

    data = data_dup(entry->value);

    key = malloc(strlen(entry->key) + 1);
    if (key == NULL)
    {
        return NULL;
    }
    strcpy(key, entry->key);

    new_entry = entry_create(key, data);

    return new_entry;
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value)
{
    if (entry != NULL && new_key != NULL && new_value != NULL)
    {
        if (entry->value != NULL)
        {
            data_destroy(entry->value);
        }
        if (entry->key != NULL)
        {
            free(entry->key);
        }
        entry->key = new_key;
        entry->value = new_value;
    }
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2)
{
    if (entry1->key != NULL && entry2->key != NULL)
    {
        int result = strcmp(entry1->key, entry2->key);
        int size1 = strlen(entry1->key);
        int size2 = strlen(entry2->key);

        if (result == 0)
        {
            return 0;
        }
        if (size1 < size2)
        {
            return -1;
        }
        return 1;
    }
    return -2;
}