/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table_skel.h"
#include "message-private.h"
#include "network_server.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "inet.h"

#include "table_server.h"

#include <pthread.h>
#include "thread-private.h"


/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port)
{
    int sockfd;
    struct sockaddr_in server;
    int sndsize;
    if (port < 0)
    {
        return -1;
    }

    sndsize = 1;

    // Cria socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket");
        return -1;
    }



    int err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sndsize, (int)sizeof(sndsize));
    if (err < 0)
    {
        if (errno == EINTR)
        {
            perror("setsockopt failed");
            return -1;
        }
    }

    // Preenche estrutura server para bind
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    // Faz bind
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    // Faz listen
    if (listen(sockfd, 0) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura MessageT.
 * - Se erro ou se cliente fechar socket retorna NULL
 */
MessageT *network_receive(int client_socket)
{
    uint8_t *buffer;
    MessageT *msg;
    int nbytes, len;
    if (client_socket < 0)
    {
        return NULL;
    }

    //Leio primeiro o tamanho do buffer que vou buscar
    if ((nbytes = read_all(client_socket, &len, sizeof(int))) == -1)
    {
        perror("Erro ao receber dados do cliente");
        close(client_socket);
        return NULL;
    }
    else if (nbytes == -2)
    {
        printf("Cliente com socket %d encerrou conexão.\n", client_socket);
        close(client_socket);
        return NULL;
    }

    len = ntohl(len);

    //Faço malloc do buffer com o tamanho len
    buffer = malloc(len);
    if (buffer == NULL)
    {
        return NULL;
    }

    //Leio o socket com o buffer em seguida
    if ((nbytes = read_all(client_socket, buffer, len)) < 0)
    {
        perror("Erro ao receber dados do cliente");
        close(client_socket);
        return NULL;
    }

    //Faço malloc da msg
    msg = malloc(sizeof(MessageT));
    if (msg == NULL)
    {
        return NULL;
    }

    //Deserializo a mensagem que está em buffer
    msg = message_t__unpack(NULL, nbytes, buffer);

    return msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg)
{
    uint8_t *buffer;
    int nbytes;
    int len, len_bytes;
    if (client_socket < 0 || msg == NULL)
    {
        return -1;
    }
    //Reservo memória para o buffer
    len = message_t__get_packed_size(msg);
    buffer = malloc(len);
    if (buffer == NULL)
    {
        return -1;
    }

    //Serializar a msg no buffer
    message_t__pack(msg, buffer);

    len_bytes = htonl(len);
    //Escrevo no socket o buffer
    if ((nbytes = write_all(client_socket, &len_bytes, sizeof(int))) != sizeof(int))
    {
        perror("Erro ao enviar resposta ao cliente");
        close(client_socket);
        return -1;
    }

    //Escrevo no socket o buffer
    if ((nbytes = write_all(client_socket, buffer, len)) != len)
    {
        perror("Erro ao enviar resposta ao cliente");
        close(client_socket);
        return -1;
    }

    return 0;
}



/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket)
{
    int connsockfd;
    struct sockaddr_in client;
    socklen_t size_client;

    pthread_t nova;

    printf("Waiting for a connection...\n");

    // Bloqueia a espera de pedidos de conexão
    while ((connsockfd = accept(listening_socket, (struct sockaddr *)&client, &size_client)) != -1)
    {
        printf("Connection done with connsockfd = %d\n", connsockfd);

        if (pthread_create(&nova, NULL, &thread_loop, (void *)&connsockfd) != 0)
        {
            perror("\nThread não criada.\n");
            exit(EXIT_FAILURE);
        }

        pthread_detach(nova);
    }
    return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close(int connectfd)
{
    return close(connectfd);
}