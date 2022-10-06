/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "serialization.h"
#include "entry.h"
#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Serializa uma estrutura data num buffer que será alocado
 * dentro da função. Além disso, retorna o tamanho do buffer
 * alocado ou -1 em caso de erro.
 */
int data_to_buffer(struct data_t *data, char **data_buf)
{
    int size;
    void* value;
    if(data == NULL || data_buf == NULL){
        return -1;
    }

    *data_buf = malloc(sizeof(int) + data->datasize);
    if(*data_buf == NULL){
        return -1;
    }

    size = data->datasize;
    value = malloc(size);
    if (value == NULL)
    {
        return -1;
    }
    memcpy(value, data->data, data->datasize);
    

    memcpy(*data_buf, &size, sizeof(int));
    memcpy(*data_buf + sizeof(int), &value, data->datasize);

    return sizeof(int) + data->datasize;
}

/* De-serializa a mensagem contida em data_buf, com tamanho
 * data_buf_size, colocando-a e retornando-a numa struct
 * data_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct data_t *buffer_to_data(char *data_buf, int data_buf_size)
{
    struct data_t* data2;
    void* data;
    int data_size;

    if (data_buf == NULL || data_buf_size <= 0)
    {
        return NULL;
    }
    data_size = 0;
    memcpy(&data_size, data_buf, sizeof(int));

    data = malloc(data_size);
    if (data == NULL)
    {
        return NULL;
    }
    memcpy(&data, data_buf + sizeof(int), data_buf_size - sizeof(int));

    data2 = data_create2(data_size, data);


    return data2;
}

/* Serializa uma estrutura entry num buffer que sera alocado
 * dentro da função. Além disso, retorna o tamanho deste
 * buffer alocado ou -1 em caso de erro.
 */
int entry_to_buffer(struct entry_t *data, char **entry_buf)
{
    int key_length;
    char* my_key;
    int data_size;
    void* value;

    if(data == NULL || entry_buf == NULL){
        return -1;
    }

    key_length = strlen(data->key) + 1;
    *entry_buf = malloc(sizeof(int) + key_length + sizeof(int) + data->value->datasize);
    if(*entry_buf == NULL) {
        return -1;
    }
    memcpy(*entry_buf, &key_length, sizeof(int));

    my_key = malloc(key_length);
    if (my_key == NULL)
    {
        return -1;
    }
    memcpy(my_key, data->key, key_length);
    memcpy(*entry_buf + sizeof(int), &(my_key), key_length);

    data_size = data->value->datasize;
    memcpy(*entry_buf + sizeof(int) + key_length, &data_size, sizeof(int));

    value = malloc(data_size);
    if (value == NULL)
    {
        return -1;
    }
    memcpy(value, data->value->data, data_size);
    memcpy(*entry_buf + sizeof(int) + key_length + sizeof(int), &value, data->value->datasize);

    return sizeof(int) + key_length + sizeof(int) + data->value->datasize;
}

/* De-serializa a mensagem contida em entry_buf, com tamanho
 * entry_buf_size, colocando-a e retornando-a numa struct
 * entry_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct entry_t *buffer_to_entry(char *entry_buf, int entry_buf_size)
{
    struct entry_t* entry2;
    int key_length;
    char* key;
    struct data_t* value;
    int value_size;
    void* data;

    if(entry_buf == NULL || entry_buf_size <= 0){
        return NULL;
    }

    key_length = 0;
    memcpy(&key_length, entry_buf, sizeof(int));

    key = malloc(key_length);
    if (key == NULL)
    {
        return NULL;
    }
    memcpy(&key, entry_buf + sizeof(int), key_length);


    value_size = 0;
    memcpy(&value_size, entry_buf + sizeof(int) + key_length, sizeof(int));

    data = malloc(value_size);
    if (data == NULL)
    {
        return NULL;
    }
    memcpy(&data, entry_buf + sizeof(int) + key_length + sizeof(int), value_size);

    value = data_create2(value_size, data);

    entry2 = entry_create(key, value);

    return entry2;
}