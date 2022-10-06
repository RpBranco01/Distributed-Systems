/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "data.h"
#include "client_stub.h"
#include "inet.h"
#include "entry.h"
#include "client_stub-private.h"
#include <signal.h>

#include "stats-private.h"

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    struct rtable_t *rtable;
    if (argc != 2)
    {
        printf("Uso: ./client <ip_servidor>:<porto_servidor>\n");
        printf("Exemplo de uso: ./client 127.0.0.1:12345\n");
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);

    printf("Iniciando conexão com zookeeper\n");

    rtable = rtable_connect(argv[1]);
    if (rtable == NULL)
    {
        printf("Não existe servidor primário\n");
        return -1;
    }

    printf("rtable_connect() done.\n");

    char *command;
    char *tokens;

    command = malloc(MAX_MSG);
    if (command == NULL)
    {
        printf("Error\n");
        return -1;
    }

    tokens = malloc(MAX_MSG);
    if (tokens == NULL)
    {
        printf("Error\n");
        return -1;
    }

    while (1)
    {
        printf("Enter a command:\n");
        fgets(command, MAX_MSG, stdin);
        tokens = strtok(command, " ");
        if (strcmp(tokens, "quit") == 0 || strcmp(tokens, "quit\n") == 0)
        {
            printf("quit()\n");
            if (rtable_disconnect(rtable) == 0)
            {
                printf("Desconexão efetuada\n");
                return -1;
            }
            return 0;
        }
        
        if (strcmp(tokens, "put") == 0 || strcmp(tokens, "put\n") == 0)
        {
            char *key, *data;
            key = strtok(NULL, " ");
            data = strtok(NULL, " ");
            if (key == NULL || data == NULL)
            {
                printf("Not enough args\n");
            }
            else
            {
                printf("%s\n", key);
                printf("%s\n", data);
                struct data_t *my_data = data_create2(strlen(data) + 1, data);
                if (my_data == NULL)
                {
                    return -1;
                }
                struct entry_t *my_entry = entry_create(key, my_data);
                if (my_entry == NULL)
                {
                    return -1;
                }

                
                if (rtable_put(rtable, my_entry) == 0)
                {
                    printf("Entry inserida com sucesso\n");
                }
                else
                {
                    printf("Erro na inserção da entry\n");
                }
            }
        }
        else if (strcmp(tokens, "get") == 0 || strcmp(tokens, "get\n") == 0)
        {
            char *key;
            key = strtok(NULL, "\n");
            if (key == NULL)
            {
                printf("Not enough args\n");
            }
            else
            {
                struct data_t *my_data = rtable_get(rtable, key);
                
                if (my_data == NULL)
                {
                    printf("Erro no get\n");
                }
                else
                {
                    printf("Value = %s\n", (char *)my_data->data);
                    if ( my_data->datasize == 0)
                    {
                        printf("Datasize = %d\n", my_data->datasize);
                    }else
                    {
                        printf("Datasize = %d\n", my_data->datasize-2);
                    }
                }
            }
        }
        else if (strcmp(tokens, "del") == 0 || strcmp(tokens, "del\n") == 0)
        {
            char *key;
            key = strtok(NULL, "\n");
            if (key == NULL)
            {
                printf("Not enough args\n");
            }
            else
            {
                
                if (rtable_del(rtable, key) == 0)
                {
                    printf("Entry removida com sucess.\n");
                }
                else
                {
                    printf("Erro na remoção da entry\n");
                }
            }
        }
        else if (strcmp(tokens, "size") == 0 || strcmp(tokens, "size\n") == 0)
        {
            
            int size = rtable_size(rtable);

            if (size == -1)
            {
                printf("Erro ao calcular o tamanho\n");
            }
            else
            {
                printf("Size = %d\n", size);
            }
        }
        else if (strcmp(tokens, "getKeys") == 0 || strcmp(tokens, "getKeys\n") == 0)
        {
            
            char **keys = rtable_get_keys(rtable);

            if (keys == NULL)
            {
                printf("A table está vazia\n");
            }
            else
            {
                int i = 0;
                while (keys[i] != NULL)
                {
                    printf("%s\n", keys[i]);
                    i++;
                }
                rtable_free_keys(keys);
            }
        }
        else if (strcmp(tokens, "table_print") == 0 || strcmp(tokens, "table_print\n") == 0)
        {
            
            rtable_print(rtable);
        }
        
        


        else if (strcmp(tokens, "stats") == 0 || strcmp(tokens, "stats\n") == 0)
        {
            struct statistics *stats = rtable_stats(rtable);
            int total_time, media, resto;
            float d_resto;
            if (stats == NULL)
            {
                printf("Erro ao procurar estatisticas.\n");
            }
            else if (stats->total_counter == 0)
            {
                printf("Não foi feita nenhuma operação.\n");
            }
            else
            {
                total_time = stats->timer_sec/1000000 + stats->timer_usec;
                media = total_time / stats->total_counter;
                resto = total_time % stats->total_counter;

                d_resto = (float) media + ((float)resto / stats->total_counter);


                printf("Total de operações: %d\n", stats->total_counter);
                printf("Operações get: %d\n", stats->counter_get);
                printf("Operações del: %d\n", stats->counter_del);
                printf("Operações put: %d\n", stats->counter_put);
                printf("Operações size: %d\n", stats->counter_size);
                printf("Operações getKeys: %d\n", stats->counter_getKeys);
                printf("Operações table_print: %d\n", stats->counter_table_print);
                printf("Tempo médio de operação: %.3f microsegundos\n", d_resto);

            }
        }
        // -----------------------------------------------------------------------------
        else
        {
            
            printf("Function not found\n");
        }
    }
}