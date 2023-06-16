all: file_io network_io bibakBoxClient bibakBoxServer 

file_io:
	gcc -c -g file_io.c -o file_io

network_io:
	gcc -c -g network_io.c -o network_io

bibakBoxClient:
	gcc -g bibakBoxClient.c -o bibakBoxClient

bibakBoxServer:
	gcc -g bibakBoxServer.c -o bibakBoxServer

clean:
	rm bibakBoxClient bibakBoxServer file_io network_io
