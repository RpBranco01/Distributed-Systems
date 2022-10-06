/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table_skel.h"
#include "entry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table_server.h"
#include "stats-private.h"
#include <zookeeper/zookeeper.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "client_stub-private.h"
#include "client_stub.h"

typedef struct String_vector zoo_string;

static char *zoo_path = "/kvstore";
static char *watcher_ctx = "ZooKeeper Data Watcher";
static zhandle_t *zh;
static int is_connected;
static int is_backup;
static int server_socket;
struct rtable_t *rtable;
/* ZooKeeper Znode Data Length (1MB, the max supported) */
#define ZDATALEN 1024 * 1024

/*  Função que verifica quando ocorre um fecho forçado do
    servidor (Crl+c).
    Função encerra conexão entre servidores e desliga o
    handler do zookeeper. */
void interrupt_watcher(int signum)
{
    if (server_socket > 0)
    {
        printf("A fechar porta de conexão entre servidores...\n");
        close(server_socket);
        printf("Porta encerrada.\n");
    }

    printf("A desligar zookeeper...\n");
    zookeeper_close(zh);
    printf("Zookeeper desligado.\n");
    exit(1);
}

/**
 * Watcher function for connection state change events
 */
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            printf("Está conectado\n");
            is_connected = 1;
        }
        else
        {
            is_connected = 0;
        }
    }
}

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    struct sockaddr_in server;
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            /* Get the updated children and reset the watch */
            if (ZOK != zoo_wget_children(zh, zoo_path, child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s 2.0!\n", zoo_path);
            }

            if (ZNONODE == zoo_exists(zh, "/kvstore/primary", 0, NULL) && ZOK == zoo_exists(zh, "/kvstore/backup", 0, NULL))
            {
                printf("Servidor primário foi encerrado. Substituindo pelo servidor secundário...\n");

                char *zoo_data = malloc(ZDATALEN * sizeof(char));
                int zoo_data_len = ZDATALEN;

                printf("Buscando address:port do servidor secundário...\n");

                zoo_get(zh, "/kvstore/backup", 0, zoo_data, &zoo_data_len, NULL);

                printf("Address:port do servidor secundário é %s...\n", zoo_data);
                printf("Apagando servidor secundário...\n");

                zoo_delete(zh, "/kvstore/backup", -1);

                printf("Servidor secundário apagado. Criando servidor primário com porta %s...\n", zoo_data);

                if (ZOK == zoo_create(zh, "/kvstore/primary", zoo_data, zoo_data_len, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                {
                    fprintf(stderr, "Servidor primário criado com sucesso.\n");
                }

                is_backup = 0;
            }
            if (ZOK == zoo_exists(zh, "/kvstore/primary", 0, NULL) && ZOK == zoo_exists(zh, "/kvstore/backup", 0, NULL))
            {
                if (is_backup == 0)
                {
                    printf("Servidor primário e secundário detetados. Criando conexão entre estes...\n");

                    char *zoo_data = malloc(ZDATALEN * sizeof(char));
                    char **list_address_port;
                    int zoo_data_len = ZDATALEN;
                    zoo_get(zh, "/kvstore/backup", 0, zoo_data, &zoo_data_len, NULL);

                    list_address_port = split_address_port(zoo_data);

                    server.sin_family = AF_INET;
                    server.sin_port = htons(atoi(list_address_port[1]));

                    if (inet_pton(AF_INET, list_address_port[0], &server.sin_addr) < 1) //É PRECISO MUDAR O HOSTNAME
                    {
                        exit(-1);
                    }

                    // Cria socket TCP para conexão entre servidores
                    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("Erro ao criar socket TCP");
                        exit(-1);
                    }

                    // Estabelece conexão com o servidor definido em server
                    if (connect(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
                    {
                        perror("Erro ao conectar-se ao servidor secundário");
                        close(server_socket);
                        exit(-1);
                    }

                    rtable = malloc(sizeof(struct rtable_t));
                    rtable->sockfd = server_socket;

                    if (table_size(table) != 0)
                    {
                        char **my_keys = table_get_keys(table);
                        struct entry_t *entry;
                        struct data_t *data;
                        for (int i = 0; i < table_size(table); i++)
                        {
                            if (my_keys[i] != NULL)
                            {
                                data = table_get(table, my_keys[i]);
                                entry = entry_create(my_keys[i], data);

                                rtable_put(rtable, entry);
                                /* entry_destroy(entry); */
                            }
                            else
                            {
                                printf("Erro no keys");
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    free(children_list);
}

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int table_skel_init(int n_lists, char *sockfd, char *zoo_ip)
{
    signal(SIGINT, interrupt_watcher);
    int len;
    char **list_address_port;
    char *address_port;
    if (n_lists <= 0)
    {
        return -1;
    }

    table = table_create(n_lists);
    if (table == NULL)
    {
        return -1;
    }

    stats = malloc(sizeof(struct statistics));
    if (stats == NULL)
    {
        return -1;
    }

    stats->counter_del = 0;
    stats->counter_get = 0;
    stats->counter_getKeys = 0;
    stats->counter_put = 0;
    stats->counter_size = 0;
    stats->counter_table_print = 0;
    stats->timer_sec = 0;
    stats->timer_usec = 0;
    stats->total_counter = 0;

    printf("A conectar com zookeeper...\n");

    zoo_set_debug_level((ZooLogLevel)1);

    zh = zookeeper_init(zoo_ip, connection_watcher, 2000, 0, 0, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
        exit(EXIT_FAILURE);
    }

    while (is_connected == 0)
    {
        // Do nothing
    }

    printf("Efeutada conexão com zookeeper!\n");

    list_address_port = split_address_port(zoo_ip);

    address_port = concat_address_port(list_address_port[0], sockfd);

    len = strlen(address_port) + 1;

    printf("Verificando se /kvstore existe...\n");

    if (is_connected == 1)
    {
        // Verifica se existe /kvstore
        if (ZOK == zoo_exists(zh, zoo_path, 0, NULL))
        {
            printf("/kvstore existe!\n");

            printf("Verificando os nós filhos de /kvstore\n");

            // Faz get children_list
            zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
            if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
            }

            // Verifica se existe servidor primário
            if (children_list->count == 1)
            {
                printf("Já existe servidor primário. Criando servidor secundário...\n");

                char *secondary_server = "/kvstore/backup";

                // Cria servidor secundário
                if (ZOK == zoo_create(zh, secondary_server, address_port, len, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                {
                    fprintf(stderr, "%s created!\n", secondary_server);
                }

                printf("Servidor secundário criado!\n");

                is_backup = 1;
            }
            else if (children_list->count == 0)
            {
                printf("Criando servidor primário...\n");
                char *primary_server = "/kvstore/primary";

                // Criei o servidor primário...
                if (ZOK == zoo_create(zh, primary_server, address_port, len, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                {
                    fprintf(stderr, "%s created!\n", primary_server);
                }

                is_backup = 0;

                printf("Servidor primário criado!\n");
            }
            else
            {
                return -2;
            }
        }
        else
        {
            printf("/kvstore não existe. Criando /kvstore...\n");

            // Cria o caminho /kvstore
            if (ZOK == zoo_create(zh, zoo_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0))
            {
                fprintf(stderr, "%s created!\n", zoo_path);
            }
            printf("/kvstore criado!\n");

            printf("Iniciando watch...\n");

            // Faz get children_list
            zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
            if (ZOK != zoo_wget_children(zh, zoo_path, &child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", zoo_path);
            }

            printf("Watch feito!!\n");
            printf("Criando servidor primário...\n");

            char *primary_server = "/kvstore/primary";

            // Cria o servidor primário
            if (ZOK == zoo_create(zh, primary_server, address_port, len, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
            {
                fprintf(stderr, "%s created!\n", primary_server);
            }

            printf("Servidor primário criado!\n");

            is_backup = 0;
        }
    }

    return 0;
}

/* Liberta toda a memória e recursos alocados pela função table_skel_init.
 */
void table_skel_destroy()
{
    table_destroy(table);
}

/* Executa uma operação na tabela (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura MessageT para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao incializada)
 */
int invoke(MessageT *msg)
{
    struct data_t *my_data;
    MessageT__Entry *my_entry;
    ProtobufCBinaryData data_temp;
    int count;
    char **list_key;
    struct entry_t *entry;

    if (msg == NULL || table == NULL)
    {
        return -1;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_PUT)
    {

        printf("DOING PUT\n");
        if (is_backup == 0)
        {
            if (ZNONODE == zoo_exists(zh, "/kvstore/backup", 0, NULL))
            {
                printf("Impossivel fazer PUT, backup inexistente\n");
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
        }

        stats->counter_put += 1;
        stats->total_counter += 1;

        printf("counter_put = %d\n", stats->counter_put);
        printf("total_counter = %d\n\n", stats->total_counter);

        my_data = data_create(msg->entries[0]->data.len);
        memcpy(my_data->data, msg->entries[0]->data.data, my_data->datasize);

        printf("Inserindo chave %s com dado %s\n", msg->entries[0]->key, (char *)my_data->data);

        if (is_backup == 0)
        {
            char *key = malloc(strlen(msg->entries[0]->key) + 1);
            strcpy(key,msg->entries[0]->key);
            entry = entry_create(key, my_data);

            printf("Pedido put de primário para secundário.\n");

            if (rtable_put(rtable, entry) == 0)
            {
                printf("Pedido feito\n");
            }
        }

        if (table_put(table, msg->entries[0]->key, my_data) == 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            printf("Insersão completa\n");
            entry_destroy(entry);
            return 0;
        }
        else
        {
            data_destroy(my_data);
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_GET)
    {

        printf("DOING GET\n");

        stats->counter_get += 1;
        stats->total_counter += 1;

        printf("counter_get = %d\n", stats->counter_get);
        printf("total_counter = %d\n\n", stats->total_counter);

        // Vou buscar à table a data que quero com a chave dada
        my_data = table_get(table, msg->entries[0]->key);
        if (my_data == NULL)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

            // Dou ao meu protobufData os valores guardados da my_data que quero meter
            data_temp.len = 0;
            data_temp.data = NULL;

            // Meto dentro da message_t->entries[0]->data a minha data
            msg->entries[0]->data = data_temp;
            return 0;
        }

        // Dou ao meu protobufData os valores guardados da my_data que quero meter
        data_temp.len = my_data->datasize;
        data_temp.data = malloc(my_data->datasize);
        memcpy(data_temp.data, my_data->data, my_data->datasize);

        // Meto dentro da message_t->entries[0]->data a minha data
        msg->entries[0]->data = data_temp;

        msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;

        return 0;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_DEL)
    {

        printf("DOING DEL\n");

        if (is_backup == 0)
        {
            if (ZNONODE == zoo_exists(zh, "/kvstore/backup", 0, NULL))
            {
                printf("Não é possível fazer DEL, backup inexistente\n");
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
        }

        stats->counter_del += 1;
        stats->total_counter += 1;

        printf("counter_del = %d\n", stats->counter_del);
        printf("total_counter = %d\n\n", stats->total_counter);

        if (is_backup == 0)
        {
            printf("Pedido delete de primário para secundário.\n");
            if (rtable_del(rtable, msg->entries[0]->key) == -1)
            {
                printf("Erro em efeutação do pedido\n");
            }
            printf("Pedido executado\n");
        }

        if (table_del(table, msg->entries[0]->key) == 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

            return 0;
        }

        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return 0;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_SIZE)
    {

        printf("DOING SIZE\n");

        stats->counter_size += 1;
        stats->total_counter += 1;

        printf("counter_size = %d\n", stats->counter_size);
        printf("total_counter = %d\n\n", stats->total_counter);

        count = table_size(table);
        if (count < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }

        msg->size_entries = count;

        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        return 0;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_GETKEYS)
    {

        printf("DOING GETKEYS\n");

        stats->counter_getKeys += 1;
        stats->total_counter += 1;

        printf("counter_getKeys = %d\n", stats->counter_getKeys);
        printf("total_counter = %d\n\n", stats->total_counter);

        count = table_size(table);
        if (count < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        list_key = table_get_keys(table);
        msg->n_entries = count;
        msg->entries = malloc(count * sizeof(MessageT__Entry *));
        my_entry = malloc(count * sizeof(MessageT__Entry));

        for (int i = 0; i < count; i++)
        {
            message_t__entry__init(&my_entry[i]);

            // Dou à minha entry o valor da key que quero meter
            my_entry[i].key = malloc(strlen(list_key[i]) + 1);
            strcpy(my_entry[i].key, list_key[i]);

            msg->entries[i] = &my_entry[i];
        }

        msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
        return 0;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_PRINT)
    {

        printf("DOING TABLE_PRINT\n");

        stats->counter_table_print += 1;
        stats->total_counter += 1;

        printf("counter_table_print = %d\n", stats->counter_table_print);
        printf("total_counter = %d\n\n", stats->total_counter);

        count = table_size(table);
        if (count < 0)
        {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        list_key = table_get_keys(table);
        msg->n_entries = count;
        msg->entries = malloc(count * sizeof(MessageT__Entry *));
        my_entry = malloc(count * sizeof(MessageT__Entry));
        for (int i = 0; i < count; i++)
        {
            message_t__entry__init(&my_entry[i]);

            // Dou à minha entry o valor da key que quero meter
            my_entry[i].key = malloc(strlen(list_key[i]) + 1);
            strcpy(my_entry[i].key, list_key[i]);

            my_data = table_get(table, list_key[i]);
            if (my_data == NULL)
            {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }

            data_temp.len = my_data->datasize;
            data_temp.data = malloc(my_data->datasize);
            memcpy(data_temp.data, my_data->data, my_data->datasize);
            my_entry[i].data = data_temp;

            msg->entries[i] = &my_entry[i];

            data_destroy(my_data);
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_PRINT + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
        return 0;
    }

    if (msg->opcode == MESSAGE_T__OPCODE__OP_STATS)
    {

        msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

        msg->stats->counter_del = stats->counter_del;
        msg->stats->counter_get = stats->counter_get;
        msg->stats->counter_getkeys = stats->counter_getKeys;
        msg->stats->counter_put = stats->counter_put;
        msg->stats->counter_size = stats->counter_size;
        msg->stats->counter_table_print = stats->counter_table_print;
        msg->stats->total_counter = stats->total_counter;
        msg->stats->timer_sec = stats->timer_sec;
        msg->stats->timer_usec = stats->timer_usec;

        return 0;
    }

    return -1;
}