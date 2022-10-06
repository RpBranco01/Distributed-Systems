/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "message-private.h"

int read_all(int sock, void *buf, int len)
{    
    int bufsize = len;
    while (len > 0)
    {

        int res = read(sock, buf, len);        
        if (res < 0)
        {
            if (errno == EINTR)
                continue;
            perror("read failed");
            return -1;
        }
        else if (res == 0)
        {
            printf("Client closing.\n");
            return -2;
        }
        
        buf += res; 
        len -= res;
    }
    return bufsize;
}

int write_all(int sock, void *buf, int len)
{

    int bufsize = len;
    while (len > 0)
    {
        int res = write(sock, buf, len);
        if (res < 0)
        {
            if (errno == EINTR)
                continue;
            perror("write failed");
            return -1;
        }    
        buf += res;
        len -= res;
    }
    return bufsize;
}