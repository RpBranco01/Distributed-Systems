/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table-private.h"
#include "list-private.h"
#include "list.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"


/* Função para criar/inicializar uma nova tabela hash, com n
 * linhas (n = módulo da função hash)
 * Em caso de erro retorna NULL.
 */
struct table_t *table_create(int n)
{
    struct table_t *table;
    if (n <= 0)
    {
        return NULL;
    }

    table = malloc(sizeof(struct table_t));
    if (table == NULL)
    {
        return NULL;
    }

    table->size = n;

    table->list = malloc(n * sizeof(struct list_t));
    if (table->list == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < n; i++)
    {
        table->list[i] = list_create();
        if (table->list[i] == NULL)
        {
            return NULL;
        }
    }

    return table;
}

/* Função para libertar toda a memória ocupada por uma tabela.
 */
void table_destroy(struct table_t *table)
{
    if (table != NULL)
    {
        if (table->list != NULL)
        {
            for (int i = 0; i < table->size; i++)
            {
                if (table->list[i] != NULL)
                {
                    list_destroy(table->list[i]);
                }
            }
        }
        free(table);
    }
}

/* Função para adicionar um par chave-valor à tabela.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na tabela,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int table_put(struct table_t *table, char *key, struct data_t *value)
{
    struct entry_t *new_entry;
    char *new_key;
    struct data_t *new_value;
    int key_size;

    if (table == NULL || key == NULL || value == NULL)
    {
        return -1;
    }

    key_size = strlen(key) + 1;
    new_key = malloc(key_size);
    if (new_key == NULL)
    {
        return -1;
    }

    strcpy(new_key, key);

    new_value = data_dup(value);
    if (new_value == NULL)
    {
        return -1;
    }

    new_entry = entry_create(new_key, new_value);

    if (hash(key, table->size) > table->size)
    {
        return -1;
    }

    return list_add((table->list[hash(key, table->size)]), new_entry);
};

/* Função para obter da tabela o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou table_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da tabela,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função.
 * Devolve NULL em caso de erro.
 */
struct data_t *table_get(struct table_t *table, char *key)
{
    struct data_t *data;
    int index;

    if (table == NULL || key == NULL)
    {
        return NULL;
    }

    index = hash(key, table->size);

    if (list_get(table->list[index], key) == NULL)
    {
        return NULL;
    }

    data = data_dup(list_get(table->list[index], key)->value);

    return data;
}

/* Função para remover um elemento da tabela, indicado pela chave key, 
 * libertando toda a memória alocada na respetiva operação table_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int table_del(struct table_t *table, char *key)
{
    int index;
    if (table == NULL || key == NULL)
    {
        return -1;
    }

    index = hash(key, table->size);
    return list_remove(table->list[index], key);
}

/* Função que devolve o número de elementos contidos na tabela.
 */
int table_size(struct table_t *table)
{
    int sum;

    if (table == NULL)
    {
        return -1;
    }

    sum = 0;
    for (int i = 0; i < table->size; i++)
    {
        if (table->list != NULL)
        {
            if (list_size(table->list[i]) == -1)
            {
                return -1;
            }
            sum += list_size(table->list[i]);
        }
    }

    return sum;
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **table_get_keys(struct table_t *table)
{
    int sum;
    int index;

    char** list_keys;
    if (table == NULL)
    {
        return NULL;
    }

    sum = 0;
    for (int i = 0; i < table->size; i++)
    {
        sum += list_size(table->list[i]);
    }

    list_keys = malloc(sum * sizeof(char*) + 8);
    if (list_keys == NULL)
    {
        return NULL;
    }

    index = 0;
    for (int i = 0; i < table->size; i++)
    {
        char** keys = list_get_keys(table->list[i]);
        for (int j = 0; j < list_size(table->list[i]); j++)
        {
            list_keys[index] = keys[j];
            index++;
        }
        free(keys);
    }

    list_keys[index] = NULL;
    return list_keys;
}

/* Função que liberta toda a memória alocada por table_get_keys().
 */
void table_free_keys(char **keys)
{
    int i;
    if (keys != NULL)
    {
        i = 0;
        while (keys[i] != NULL)
        {
            free(keys[i]);
            i++;
        }

        free(keys);
    }
}

/* Função que imprime o conteúdo da tabela.
 */
void table_print(struct table_t *table)
{
    if (table != NULL)
    {
        printf("[");
        for (int i = 0; i < table->size; i++)
        {
            list_print(table->list[i]);

            if (i < table->size - 1)
            {
                printf(",\n");
            }
        }
        printf("]\n");
    }
}