/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_server.h"
#include "message-private.h"
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable)
{
    struct sockaddr_in server;
    int sockfd;
    if (rtable == NULL)
    {
        return -1;
    }
    server = rtable->address;

    // Cria socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    // Estabelece conexão com o servidor definido em server
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao conectar-se ao servidor");
        close(sockfd);
        return -1;
    }

    rtable->sockfd = sockfd;

    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtable_t;
 * - Reservar memória para serializar a mensagem contida em msg;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Libertar a memória ocupada pela mensagem serializada enviada;
 * - Esperar a resposta do servidor;
 * - Reservar a memória para a mensagem serializada recebida;
 * - De-serializar a mensagem de resposta, reservando a memória 
 *   necessária para a estrutura MessageT que é devolvida;
 * - Libertar a memória ocupada pela mensagem serializada recebida;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtable_t *rtable, MessageT *msg)
{
    int sockfd;
    uint8_t *buffer;
    int len, nbytes, len_bytes;
    MessageT *new_msg;
    if (rtable == NULL || msg == NULL)
    {
        return NULL;
    }
    //Guardo o socket numa variável
    sockfd = rtable->sockfd;


    //Faço malloc no buffer com o tamanho de msg
    len = message_t__get_packed_size(msg);
    buffer = malloc(len);
    if (buffer == NULL)
    {
        return NULL;
    }

    //Serializo a msg no buffer
    message_t__pack(msg, buffer);

    //Escrevo tudo de uma vez no socket (Tamanho e buffer)
    len_bytes = htonl(len);
    if ((nbytes = write_all(sockfd, &len_bytes, sizeof(int))) != sizeof(int))
    {
        perror("Erro ao enviar resposta ao servidor (tamanho do buffer)");
        close(sockfd);
        return NULL;
    }
    

    if ((nbytes = write_all(sockfd, buffer, len)) != len)
    {
        perror("Erro ao enviar resposta ao servidor");
        close(sockfd);
        return NULL;
    }

    //Liberto o buffer e faço malloc novamente
    free(buffer);

    //Leio primeiro o tamanho do buffer que vou buscar
    if ((nbytes = read_all(sockfd, &len_bytes, sizeof(int))) < 0)
    {
        perror("Erro ao receber dados do servidor (tamanho do buffer)");
        close(sockfd);
        return NULL;
    }

    len = ntohl(len_bytes);
    //Faço malloc do buffer com o tamanho len
    buffer = malloc(len);
    if (buffer == NULL)
    {
        return NULL;
    }

    //Leio o socket com o buffer em seguida
    if ((nbytes = read_all(sockfd, buffer, len)) < 0)
    {
        perror("Erro ao receber dados do servidor (buffer)");
        close(sockfd);
        return NULL;
    }

    //Faço malloc da nova mensagem
    new_msg = malloc(sizeof(MessageT));
    if (new_msg == NULL)
    {
        return NULL;
    }

    //Deserializo a mensagem contida em buffer para a nova mensagem
    new_msg = message_t__unpack(NULL, nbytes, buffer);
    //Liberto o buffer usado
    free(buffer);

    //Retorno a mensagem
    return new_msg;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtable_t *rtable)
{
    return close(rtable->sockfd);
}