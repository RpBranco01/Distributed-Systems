/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "data.h"
#include "stdlib.h"
#include "string.h"

/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size 
 */
struct data_t *data_create(int size){
    struct data_t* new_data;
    if(size <= 0){
        return NULL;
    }
    new_data = malloc(sizeof(struct data_t));
    if(new_data == NULL){
        return NULL;
    }
    new_data->datasize = size;
    new_data->data = malloc(size);
    if(new_data->data == NULL){
        return NULL;
    }
    return new_data;
}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data){
    struct data_t* new_data;
    if(size <= 0 || data == NULL){
        return NULL;
    }
    new_data = malloc(sizeof(struct data_t));
    if(new_data == NULL){
        return NULL;
    }
    new_data->data = data;
    new_data->datasize = size; 
    return new_data;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data){
    if(data != NULL){
        if(data->data != NULL){
            free(data->data);
        }
        free(data);
    }
}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 */
struct data_t *data_dup(struct data_t *data){
    void* data2;
    struct data_t* new_data;
    if(data == NULL || data->datasize <= 0 || data->data == NULL){
        return NULL;
    }
    data2 = malloc(data->datasize);
    if(data2 == NULL){
        return NULL;
    }
    memcpy(data2, data->data, data->datasize);
    new_data = data_create2(data->datasize, data2);
    return new_data;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data){
    if(data != NULL && new_size > 0 && new_data != NULL){
        if(data->data != NULL){
            free(data->data);
        }
        data->datasize = new_size;
        data->data = new_data; 
    }
}