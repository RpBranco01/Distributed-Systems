/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table-private.h"
#include "sdmessage.pb-c.h"
#include <netinet/in.h>

struct rtable_t
{
    struct sockaddr_in address; 
    int sockfd;
};

/*  Função que divide uma string do tipo 127.0.0.1:1234 em um array com o
    address e o port. Em [0] está o address e em [1] está o port. */
char **split_address_port(const char *address_port);

/*  Função que une um address e um port para o tipo address:port. */
char *concat_address_port(char *address, char *port);
