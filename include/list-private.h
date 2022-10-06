/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H

#include "list.h"

#include <pthread.h>

struct node_t
{
    struct entry_t *entry;
    struct node_t *next;
};

struct list_t
{
    struct node_t *head;
    pthread_mutex_t m;
    pthread_cond_t c;
    int list_size;
    int writing;
    int reading;
};

void node_destroy(struct node_t *node);

void comeca_write(struct list_t *list);

void termina_write(struct list_t *list);

void comeca_read(struct list_t *list);

void termina_read(struct list_t *list);

#endif
