/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "list.h"
#include "list-private.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#include <pthread.h>

/* Função que cria uma nova lista (estrutura list_t a ser definida pelo
 * grupo no ficheiro list-private.h).
 * Em caso de erro, retorna NULL.
 */
struct list_t *list_create()
{
    struct list_t *new_list = malloc(sizeof(struct list_t));
    if (new_list == NULL)
    {
        return NULL;
    }
    new_list->head = NULL;
    pthread_mutex_init(&new_list->m, NULL);
    pthread_cond_init(&new_list->c, NULL);
    new_list->list_size = 0;
    new_list->writing = 1;
    new_list->reading = 0;
    return new_list;
}

/* Função que elimina uma lista, libertando *toda* a memoria utilizada
 * pela lista.
 */
void list_destroy(struct list_t *list)
{
    struct node_t *no_atual;
    struct node_t *no_anterior;

    if (list != NULL)
    {
        if (list->head != NULL)
        {
            no_atual = list->head;
            while (no_atual != NULL)
            {
                entry_destroy(no_atual->entry);
                no_anterior = no_atual;
                no_atual = no_atual->next;
                free(no_anterior);
            }
        }
        free(list);
    }
}

/* Função que adiciona no final da lista (tail) a entry passada como
* argumento caso não exista na lista uma entry com key igual àquela
* que queremos inserir.
* Caso exista, os dados da entry (value) já existente na lista serão
* substituídos pelos os da nova entry.
* Retorna 0 (OK) ou -1 (erro).
*/

int list_add(struct list_t *list, struct entry_t *entry)
{

    comeca_write(list);

    struct node_t *node, *no_atual, *no_anterior;

    if (list == NULL || entry == NULL)
    {
        termina_write(list);
        return -1;
    }

    if (list->head == NULL)
    {
        node = malloc(sizeof(struct node_t));
        if (node == NULL)
        {
            termina_write(list);
            return -1;
        }
        node->entry = entry;
        node->next = NULL;
        list->head = node;
        list->list_size++;
        termina_write(list);
        return 0;
    }

    no_atual = list->head;
    while (no_atual != NULL)
    {
        if (entry_compare(no_atual->entry, entry) == 0)
        {
            no_atual->entry = entry;
            termina_write(list);
            return 0;
        }
        no_anterior = no_atual;
        no_atual = no_atual->next;
    }

    no_anterior->next = malloc(sizeof(struct node_t));
    if (no_anterior->next == NULL)
    {
        termina_write(list);
        return -1;
    }
    no_anterior->next->next = NULL;
    no_anterior->next->entry = entry;

    list->list_size++;

    termina_write(list);
    return 0;
}

/* Função que elimina da lista a entry com a chave key.
 * Retorna 0 (OK) ou -1 (erro).
 */
int list_remove(struct list_t *list, char *key)
{
    comeca_write(list);

    struct node_t *no_atual, *no_anterior;

    if (list == NULL || key == NULL)
    {
        termina_write(list);
        return -1;
    }

    if (list->head == NULL || list->head->entry == NULL)
    {
        termina_write(list);
        return -1;
    }

    if (strcmp(list->head->entry->key, key) == 0)
    {
        no_atual = list->head;
        list->head = list->head->next;
        node_destroy(no_atual);
        list->list_size--;
        termina_write(list);
        return 0;
    }

    no_anterior = list->head;
    no_atual = no_anterior->next;

    while (no_atual != NULL)
    {
        if (no_atual->entry == NULL)
        {
            termina_write(list);
            return -1;
        }
        if (strcmp(no_atual->entry->key, key) == 0)
        {
            no_anterior->next = no_atual->next;
            node_destroy(no_atual);
            list->list_size--;
            termina_write(list);
            return 0;
        }

        no_anterior = no_atual;
        no_atual = no_atual->next;
    }

    termina_write(list);

    return -1;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL em caso de erro.
 * Obs: as funções list_remove e list_destroy vão libertar a memória
 * ocupada pela entry ou lista, significando que é retornado NULL
 * quando é pretendido o acesso a uma entry inexistente.
*/
struct entry_t *list_get(struct list_t *list, char *key)
{
    comeca_read(list);

    struct node_t *no_atual;

    if (list == NULL || key == NULL)
    {
        termina_read(list);
        return NULL;
    }

    no_atual = list->head;

    while (no_atual != NULL)
    {
        if (strcmp(no_atual->entry->key, key) == 0)
        {
            termina_read(list);
            return no_atual->entry;
        }

        no_atual = no_atual->next;
    }
    termina_read(list);
    return NULL;
}

/* Função que retorna o tamanho (número de elementos (entries)) da lista,
 * ou -1 (erro).
 */
int list_size(struct list_t *list)
{
    return list->list_size;
}

/* Função que devolve um array de char* com a cópia de todas as keys da 
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **list_get_keys(struct list_t *list)
{
    comeca_read(list);

    char **keys;
    struct node_t *no_atual;
    int index;

    if (list == NULL)
    {
        return NULL;
    }

    keys = malloc((list_size(list) * sizeof(char *)) + 8);
    if (keys == NULL)
    {
        termina_read(list);
        return NULL;
    }

    no_atual = list->head;

    index = 0;
    while (no_atual != NULL)
    {
        if (no_atual->entry != NULL)
        {
            keys[index] = no_atual->entry->key;
        }
        index++;
        no_atual = no_atual->next;
    }

    keys[index] = NULL;

    termina_read(list);
    return keys;
}

/* Função que liberta a memória ocupada pelo array das keys da tabela,
 * obtido pela função list_get_keys.
 */
void list_free_keys(char **keys)
{
    int index;
    if (keys != NULL)
    {
        index = 0;
        while (keys[index] != NULL)
        {
            free(keys[index]);
            index++;
        }
        free(keys);
    }
}

/* Função que imprime o conteúdo da lista para o terminal.
 */
void list_print(struct list_t *list)
{

    struct node_t *node;

    if (list != NULL)
    {
        if (list->head == NULL)
        {
            printf("[] -> lista vazia");
        }
        else
        {
            node = malloc(sizeof(struct node_t));
            if (node != NULL)
            {
                node = list->head;

                printf("[");
                while (node != NULL)
                {
                    printf("{");
                    printf("%s: ", node->entry->key);
                    printf("(%d,%s)", node->entry->value->datasize, (char *)node->entry->value->data);
                    if (node->next != NULL)
                    {
                        printf("}, ");
                    }

                    node = node->next;
                }
                printf("}]");
            }
        }
    }
}