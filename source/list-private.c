/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "list-private.h"
#include <stdlib.h>

void node_destroy(struct node_t *node)
{
    if (node != NULL)
    {
        if (node->entry != NULL)
        {
            entry_destroy(node->entry);
        }
        free(node);
    }
}

void comeca_write(struct list_t *list){
    pthread_mutex_lock(&list->m);
    while (list->writing == 0 || list->reading > 0)
    {
        pthread_cond_wait(&list->c, &list->m);
    }
    list->writing--;
    pthread_mutex_unlock(&list->m);
}

void termina_write(struct list_t *list){
    pthread_mutex_lock(&list->m);
    list->writing++;
        pthread_cond_broadcast(&list->c);
    pthread_mutex_unlock(&list->m);
}

void comeca_read(struct list_t *list){
    pthread_mutex_lock(&list->m);
    while (list->writing == 0)
    {
        pthread_cond_wait(&list->c, &list->m);
    }
    list->reading++;
    pthread_mutex_unlock(&list->m);
}

void termina_read(struct list_t *list){
    pthread_mutex_lock(&list->m);
        list->reading--;
    pthread_mutex_unlock(&list->m);
}

