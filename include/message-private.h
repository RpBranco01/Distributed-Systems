/*  Grupo 33 - Sistemas Distribuídos 
    Rodrigo Branco FC54457
    Vasco Lopes FC54410
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> 
#include <arpa/inet.h>
#include <errno.h>

/*  Enquanto socket fd não estiver vazio, vai ler
    todos os dados de tamanho len e escrever no
    buffer buf */
int read_all(int sock, void *buf, int len);

/*  Enquanto socket fd não estiver vazio, vai escrever
    todos os dados de tamanho len que estão no buffer
    buf */
int write_all(int sock, void *buf, int len);