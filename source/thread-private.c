/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include "table_skel.h"
#include "message-private.h"

#include "network_server.h"

#include "table_server.h"
#include "stats-private.h"

#include <pthread.h>
#include <sys/time.h>

struct statistics *stats;

pthread_mutex_t m_time = PTHREAD_MUTEX_INITIALIZER;

void * thread_loop(void * cliente){
    int connsockfd = *(int *)cliente;
    MessageT *msg;
    int result;
    struct timeval begin, end;

    while ((msg = network_receive(connsockfd)) != NULL)
    {
        gettimeofday(&begin, 0);

        result = invoke(msg);

        gettimeofday(&end, 0);

        pthread_mutex_lock(&m_time);

        if (msg->opcode != MESSAGE_T__OPCODE__OP_STATS + 1)
        {
            stats->timer_sec += end.tv_sec - begin.tv_sec;
            stats->timer_usec += end.tv_usec - begin.tv_usec;
        }

        pthread_mutex_unlock(&m_time);

        if (result == 0)
        {
            network_send(connsockfd, msg);
        }
    }

    close(connsockfd);

    return NULL;
}