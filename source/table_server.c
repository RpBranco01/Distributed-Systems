/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table.h"
#include "inet.h"
#include "network_server.h"
#include "table_skel.h"
#include "table_server.h"
#include <signal.h>

#include "stats-private.h"

struct table_t *table;
struct statistics *stats;

int main(int argc, char **argv)
{
    int sockfd, skel_result;
    // Verifica se foi passado algum argumento
    if (argc != 4)
    {
        printf("Uso: ./server <porto_servidor> <numero_lista>\n");
        printf("Exemplo de uso: ./server 1234 2 127.0.0.1:2181\n");
        return -1;
    }

    /* signal(SIGPIPE, SIG_IGN); */

    sockfd = network_server_init(atoi(argv[1]));
    if (sockfd == -1)
    {
        return -1;
    }

    skel_result = table_skel_init(atoi(argv[2]), argv[1], argv[3]);
    if (skel_result == -1)
    {
        perror("Erro ao criar tabela");
        close(sockfd);
        return -1;
    }else if (skel_result == -2)
    {
        printf("Erro ao conectar com zookeeper: dois servidores ativos");
        close(sockfd);
        return -1;
    }
    
    printf("Opening server...\n");

    network_main_loop(sockfd);

    table_skel_destroy();
}