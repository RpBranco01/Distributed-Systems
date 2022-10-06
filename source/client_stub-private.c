/*  Grupo 33 - Sistemas Distribuídos
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "client_stub-private.h"
#include <string.h>
#include <stdlib.h>

char **split_address_port(const char *address_port)
{
    int total_len, address_len, port_len, i, j;
    char *port, *address;
    char **list;

    total_len = strlen(address_port) + 1;
    i = 0;
    address_len = 0;
    port_len = 0;

    while (i < total_len)
    {
        if (address_port[i] == ':')
        {
            break;
        }
        else
        {
            address_len++;
            i++;
        }
    }

    address_len = address_len + 1;
    port_len = total_len - address_len;

    address = malloc(address_len);
    port = malloc(port_len);

    i = 0;
    while (i < address_len-1)
    {
        if (address_port[i] == ':')
        {
            i++;
        }
        else
        {
            address[i] = address_port[i];
            i++;
        }
    }
    address[i] = '\0';

    j = 0;
    while (i < total_len-1)
    {
        if (address_port[i] == ':')
        {
            i++;
        }
        else
        {
            port[j] = address_port[i];
            j++;
            i++;
        }
    }

    port[j] = '\0';

    list = malloc(2 * sizeof(char *));
    list[0] = malloc(address_len);
    list[1] = malloc(port_len);

    strcpy(list[0], address);
    strcpy(list[1], port);

    free(address);
    free(port);

    return list;
}

char *concat_address_port(char *address, char *port){
    int total_len;
    char *address_port, *separator = ":";

    total_len = strlen(address) + strlen(separator) + strlen(port) + 3;

    address_port = malloc(total_len);

    strcpy(address_port, address);
    strcat(address_port, separator);
    strcat(address_port, port);

    return address_port;
}