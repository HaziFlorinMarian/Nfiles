#fisier folosit pentru compilarea serverului&clientului

all:
	gcc Server.c -o Server
	gcc Client.c -o Client
clean:
	rm -f Client Server
