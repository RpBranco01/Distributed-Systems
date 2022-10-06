OBJ_DIR = object
OBJETOS = data.o entry.o list-private.o list.o serialization.o table-private.o table.o

TABLE_CLIENT = $(OBJETOS) table_client.o client_stub.o network_client.o message-private.o sdmessage.pb-c.o client_stub-private.o
TABLE_SERVER = $(OBJETOS) table_server.o network_server.o table_skel.o message-private.o sdmessage.pb-c.o thread-private.o client_stub.o network_client.o client_stub-private.o

table_client.o = client_stub.h data.h entry.h inet.h 
client_stub.o = data.h entry.h
network_client.o = client_stub.h sdmessage.pb-c.h
network_server.o = table_skel.h
table_skel.o = sdmessage.pb-c.h table.h
table_server.o = table.h inet.h network_server.h table_skel.h
message-private.o =
sdmessage.pb-c.o = 
thread-private.o =
client_stub-private.o = table-private.h sdmessage.pb-c.h

data.o =
entry.o = data.h
list-private.o = list.h
list.o = data.h entry.h
serialization.o = data.h entry.h
table-private.o = table.h
table.o = data.h list.h
CC = gcc -Wall -g

vpath %.o $(OBJ_DIR)

all: table_client table_server client-lib.o

table_client: $(TABLE_CLIENT)
		$(CC) -D THREADED $(addprefix $(OBJ_DIR)/,$(TABLE_CLIENT)) -I/include -L/lib -pthread -lprotobuf-c -o  binary/table_client -lzookeeper_mt

table_server: $(TABLE_SERVER)
		$(CC) -D THREADED $(addprefix $(OBJ_DIR)/,$(TABLE_SERVER)) -I/include -L/lib -pthread -lprotobuf-c -o binary/table_server -lzookeeper_mt

%.o: source/%.c $($@)
		$(CC) -D THREADED -I include -o $(OBJ_DIR)/$@ -c $<

client-lib.o: source/client_stub.c source/network_client.c source/data.c source/entry.c
		ld -r object/client_stub.o object/network_client.o object/data.o object/entry.o -o lib/client-lib.o

CLEAN_OBJS = $(OBJETOS) table_client.o client_stub.o network_client.o network_server.o table_skel.o table_server.o message-private.o sdmessage.pb-c.o thread-private.o
CLEAN_EXECS = binary/table_client binary/table_server		

clean:
	rm -f $(addprefix $(OBJ_DIR)/,$(CLEAN_OBJS)) lib/client-lib.o $(CLEAN_EXECS) *pb-c.[ch]

