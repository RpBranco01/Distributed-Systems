/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "data.h"
#include "entry.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include "serialization.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>

#include "stats-private.h"
#include <zookeeper/zookeeper.h>
typedef struct String_vector zoo_string;

static char *zoo_path = "/kvstore";
static char *watcher_ctx = "ZooKeeper Data Watcher";
static char *primary_server = "/kvstore/primary";
char *primary_address_port;
char *client_address;
static int is_connected;
static zhandle_t *zh;
struct rtable_t *new_table;

#define ZDATALEN 1024 * 1024

/**
 * Watcher function for connection state change events
 */
void connection_watcher_2(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            is_connected = 1;
        }
        else
        {
            is_connected = 0;
        }
    }
}

void child_watcher_client(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    char *zoo_data = malloc(ZDATALEN * sizeof(char));
    int zoo_data_len = ZDATALEN;
    char **list_primary_address_port;
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            /* Get the updated children and reset the watch */
            if (ZOK != zoo_wget_children(zh, zoo_path, child_watcher_client, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s 2.0!\n", zoo_path);
            }

            if (ZOK == zoo_exists(zh, "/kvstore/primary", 0, NULL))
            {
                zoo_get(zh, primary_server, 0, zoo_data, &zoo_data_len, NULL);

                if (strcmp(zoo_data, primary_address_port) != 0)
                {
                    free(primary_address_port);
                    primary_address_port = malloc(strlen(zoo_data) + 1);

                    strcpy(primary_address_port, zoo_data);
                    close(new_table->sockfd);

                    list_primary_address_port = split_address_port(primary_address_port);
                    new_table->address.sin_family = AF_INET;
                    new_table->address.sin_port = htons(atoi(list_primary_address_port[1]));
                    if (inet_pton(AF_INET, client_address, &new_table->address.sin_addr) < 1)
                    {
                        exit(-1);
                    }
                    printf("Iniciando conexão com novo servidor\n");
                    printf("Enter a command:\n");
                    network_connect(new_table);
                }
            }
        }
    }
    free(zoo_data);
}

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtable_t *rtable_connect(const char *address_port)
{
    char *zoo_data;
    int result, zoo_data_len;
    char **list_primary_address_port, **list_address_port;

    if (address_port == NULL)
    {
        return NULL;
    }

    new_table = malloc(sizeof(struct rtable_t));
    if (new_table == NULL)
    {
        return NULL;
    }

    list_address_port = split_address_port(address_port);

    client_address = malloc(ZDATALEN);
    strcpy(client_address, list_address_port[0]);

    zoo_set_debug_level((ZooLogLevel)1);

    zh = zookeeper_init(address_port, connection_watcher_2, 2000, 0, 0, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
        exit(EXIT_FAILURE);
    }

    if (ZOK == zoo_exists(zh, zoo_path, 0, NULL))
    {
        zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
        if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher_client, watcher_ctx, children_list))
        {
            fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
        }

        if (ZOK == zoo_exists(zh, primary_server, 0, NULL))
        {
            printf("Existe servidor primário!\n");

            primary_address_port = malloc(ZDATALEN * sizeof(char *));
            zoo_data = malloc(ZDATALEN * sizeof(char *));
            zoo_data_len = ZDATALEN;

            zoo_get(zh, primary_server, 0, zoo_data, &zoo_data_len, NULL);
            strcpy(primary_address_port, zoo_data);

            list_primary_address_port = split_address_port(primary_address_port);
            printf("Address:Port do servidor primário é %s\n", zoo_data);

            new_table->address.sin_family = AF_INET;
            new_table->address.sin_port = htons(atoi(list_primary_address_port[1]));
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }

    if (inet_pton(AF_INET, list_address_port[0], &new_table->address.sin_addr) < 1)
    {
        return NULL;
    }

    result = network_connect(new_table);
    if (result < 0)
    {
        printf("network_connect deu erro\n");
        return NULL;
    }

    return new_table;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable)
{
    if (rtable != NULL)
    {
        network_close(rtable);
        free(rtable);
        free(primary_address_port);
        return 0;
    }
    return -1;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry)
{
    MessageT *message_t;
    MessageT__Entry *my_entry;
    ProtobufCBinaryData data_temp;
    if (rtable == NULL || entry == NULL)
    {
        return -1;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_PUT;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    // Reservo memória para message_t->entries e digo que o número de entries é 1
    message_t->n_entries = 1;
    message_t->entries = malloc(sizeof(MessageT__Entry *));

    // Reservo memória para a entry que criei e inicializo-a
    my_entry = malloc(sizeof(MessageT__Entry));
    message_t__entry__init(&my_entry[0]);

    // Dou à minha entry o valor da key que quero meter
    my_entry[0].key = malloc(strlen(entry->key) + 1);
    strcpy(my_entry[0].key, entry->key);

    // Dou ao meu protobufData os valores guardados da entry que quero meter
    data_temp.len = entry->value->datasize;
    data_temp.data = malloc(entry->value->datasize);
    memcpy(data_temp.data, entry->value->data, entry->value->datasize);

    // Meto o protobufData dentro da minha entry que criei
    my_entry[0].data = data_temp;

    // Meto dentro da message_t->entries[0] a minha entry
    message_t->entries[0] = &my_entry[0];

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    // Se op correu bem então retorno 0
    if (message_t->opcode == MESSAGE_T__OPCODE__OP_PUT + 1)
    {
        return 0;
    }

    return -1;
}

/* Função para obter um elemento da tabela.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key)
{
    MessageT *message_t;
    struct data_t *data;
    MessageT__Entry *my_entry;
    if (rtable == NULL || key == NULL)
    {
        return NULL;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_GET;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_KEY;

    // Reservo memória para message_t->entries e digo que o número de entries é 1
    message_t->n_entries = 1;
    message_t->entries = malloc(sizeof(MessageT__Entry *));

    // Reservo memória para a entry que criei e inicializo-a
    my_entry = malloc(sizeof(MessageT__Entry));
    message_t__entry__init(&my_entry[0]);

    // Dou à minha entry o valor da key que quero meter
    my_entry[0].key = malloc(strlen(key) + 1);
    strcpy(my_entry[0].key, key);

    // Meto dentro da message_t->entries[0] a minha entry
    message_t->entries[0] = &my_entry[0];

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    // Se op correu bem então envio para table_client o data
    if (message_t->opcode == MESSAGE_T__OPCODE__OP_GET + 1)
    {
        // Faço malloc da variável data que criei
        data = malloc(sizeof(struct data_t));

        // Meto dentro de data os valores que estão na message_t->entries[0]
        data->datasize = message_t->entries[0]->data.len;
        data->data = malloc(data->datasize);
        memcpy(data->data, message_t->entries[0]->data.data, message_t->entries[0]->data.len);

        return data;
    }

    return NULL;
}

/* Função para remover um elemento da tabela. Vai libertar
 * toda a memoria alocada na respetiva operação rtable_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtable_del(struct rtable_t *rtable, char *key)
{
    MessageT *message_t;
    MessageT__Entry *my_entry;
    if (rtable == NULL || key == NULL)
    {
        return -1;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_DEL;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_KEY;

    // Reservo memória para message_t->entries e digo que o número de entries é 1
    message_t->n_entries = 1;
    message_t->entries = malloc(sizeof(MessageT__Entry *));

    // Reservo memória para a entry que criei e inicializo-a
    my_entry = malloc(sizeof(MessageT__Entry));
    message_t__entry__init(&my_entry[0]);

    // Dou à minha entry o valor da key que quero meter
    my_entry[0].key = malloc(strlen(key) + 1);
    strcpy(my_entry[0].key, key);

    // Meto dentro da message_t->entries[0] a minha entry
    message_t->entries[0] = &my_entry[0];

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    // Se op correu bem então não preciso retornar nada a não ser 0
    if (message_t->opcode == MESSAGE_T__OPCODE__OP_DEL + 1)
    {
        return 0;
    }

    return -1;
}

/* Devolve o número de elementos contidos na tabela.
 */
int rtable_size(struct rtable_t *rtable)
{
    MessageT *message_t;
    if (rtable == NULL)
    {
        return -1;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    if (message_t == NULL)
    {
        perror("NULL");
        close(rtable->sockfd);
        return -1;
    }
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_SIZE;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    // Se op correu bem então devolvo o número de elementos da table = número de entradas guardadas
    if (message_t->opcode == MESSAGE_T__OPCODE__OP_SIZE + 1)
    {
        return message_t->size_entries;
    }

    return -1;
}

/* Devolve um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento a NULL.
 */
char **rtable_get_keys(struct rtable_t *rtable)
{
    MessageT *message_t;
    char **my_keys;
    if (rtable == NULL)
    {
        return NULL;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    // Se op correu bem então guardo os valores de keys numa variável
    if (message_t->opcode == MESSAGE_T__OPCODE__OP_GETKEYS + 1)
    {
        // Reservo memória para o array de strings
        my_keys = malloc(message_t->n_entries * sizeof(char *) + 8);
        for (int i = 0; i < message_t->n_entries; i++)
        {
            // Para cada string reservo memória com o tamanho da key que vou guardar
            my_keys[i] = malloc(strlen(message_t->entries[i]->key) + 1);
            strcpy(my_keys[i], message_t->entries[i]->key);
        }
        //Última posição do array = NULL
        my_keys[message_t->n_entries] = NULL;

        return my_keys;
    }

    return NULL;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys)
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

/* Função que imprime o conteúdo da tabela remota para o terminal.
 */
void rtable_print(struct rtable_t *rtable)
{
    MessageT *message_t;
    int count;
    if (rtable != NULL)
    {

        // Faço malloc do ponteiro mensagem de pedido e inicializo
        message_t = malloc(sizeof(MessageT));
        message_t__init(message_t);

        // Atribuo valores ao op_code e ao ct_type
        message_t->opcode = MESSAGE_T__OPCODE__OP_PRINT;
        message_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;

        // Envio a mensagem de pedido e recebo uma mensagem de resposta
        message_t = network_send_receive(rtable, message_t);
        if (message_t == NULL)
        {
            printf("Servidor fechado!\n");
            exit(0);
        }

        count = message_t->n_entries;
        printf("(");
        for (int i = 0; i < count; i++)
        {
            printf("\n[");
            printf("%s: ", message_t->entries[i]->key);
            printf("(%d,%s)", (int)message_t->entries[i]->data.len, (char *)message_t->entries[i]->data.data);
            printf("]\n");
        }
        printf(")\n");
    }
}

/* Obtém as estatísticas do servidor.*/
struct statistics *rtable_stats(struct rtable_t *rtable)
{

    MessageT *message_t;
    MessageT__Statistics *stats;
    struct statistics *my_stats;
    if (rtable == NULL)
    {
        return NULL;
    }

    // Faço malloc do ponteiro mensagem de pedido e inicializo
    message_t = malloc(sizeof(MessageT));
    message_t__init(message_t);

    // Atribuo valores ao op_code e ao ct_type
    message_t->opcode = MESSAGE_T__OPCODE__OP_STATS;
    message_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;

    // Reservo memória para a entry que criei e inicializo-a
    stats = malloc(sizeof(MessageT__Statistics));
    message_t__statistics__init(&stats[0]);

    message_t->stats = &stats[0];

    // Envio a mensagem de pedido e recebo uma mensagem de resposta
    message_t = network_send_receive(rtable, message_t);
    if (message_t == NULL)
    {
        printf("Servidor fechado!\n");
        exit(0);
    }

    if (message_t->opcode == MESSAGE_T__OPCODE__OP_STATS + 1)
    {
        my_stats = malloc(sizeof(struct statistics));
        if (my_stats == NULL)
        {
            return NULL;
        }

        my_stats->counter_del = message_t->stats->counter_del;
        my_stats->counter_put = message_t->stats->counter_put;
        my_stats->counter_get = message_t->stats->counter_get;
        my_stats->counter_getKeys = message_t->stats->counter_getkeys;
        my_stats->counter_size = message_t->stats->counter_size;
        my_stats->counter_table_print = message_t->stats->counter_table_print;
        my_stats->total_counter = message_t->stats->total_counter;
        my_stats->timer_sec = message_t->stats->timer_sec;
        my_stats->timer_usec = message_t->stats->timer_usec;

        return my_stats;
    }

    return NULL;
}
