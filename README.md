# Grupo 33 - Sistemas Distribuídos

    -Rodrigo Branco FC54457
    -Vasco Lopes FC54410

## Processo de compilação

Usar o comando 

> `make clean`

para limpar possiveis objetos criados anteriormente (comando opcional).

Usar comando

> `make`

E na seguinte ordem ligar o servidor

>`./binary/table_server 1234 5 127.0.0.1:2181`

E o cliente

> `./binary/table_client 127.0.0.1:2181`

#
## Explicação

### Servidores

Quando arranco o servidor dou como argumentos a porta deste, o número de listas, e o IP:Porta do zookeeper. No `table_skel_init` inicio a ligação entre o servidor e o zookeeper e verifico se existe um path `/kvstore`. De seguida verifico se existe espaço para criar o servidor, se não existir `/kvstore/primary` eu vou criar um nó com esse caminho guardado dentro do nó o IP:Porta que introduzi como argumentos. Ou seja se ligar o servidor como demonstrado acima, os meta-dados serão `127.0.0.1:1234`. A mesma coisa se for backup.

De lembrar que assim que verifico se existe `/kvstore` ou quando crio este caminho, eu faço um `zoo_wget_children` para fazer watch sobre o root_path.

Quando backup é criado o watch é ativo e faço a ligação entre o primário e o secundário. Para tal, o servidor primário cria o socket de ligação e faz `zoo_get` para ir
buscar os meta-dados do secundário e assim fazer `connect`.


### Clientes

Um cliente é ligado da forma descrita acima onde a porta referida pertence ao zookeeper. Com tal este também faz a ligação com o zookeeper. Depois verifica se existe `/kvstore` e se tal ocorrer então ativa um watch no root_path. De seguida vai verificar se existe um caminho `/kvstore/primary` para fazer a conexão com o servidor primário. Se ocorrer alterações no servidor primário o cliente é notificado e vai comparar a primeira porta que obteve do servidor primário (guardado numa variável global) com a porta que obteve a partir de `zoo_get`. Se for diferente este desliga a conexão que tinha com a porta anterior e cria uma conexão nova com a que tem atualmente.

É possível correr vários clientes em simultâneo.
#

## Extras

Para na altura de correr o código o terminal não ficar cheio de logs do zookeeper utilizei a função `zoo_set_debug_level` e deixei o `ZooLogLevel = 1`, assim apenas aparece logs de erros do zookeeper.

Como recomendado pelos professores foi utilizado o comando `-D THREADED` para retirar os warnings relativos ao zookeeper.
"# Distributed-Systems" 
