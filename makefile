all: file_io network_io bibakBoxClient bibakBoxServer 

file_io:
	gcc -c -g file_io.c -o file_io

network_io:
	gcc -c -g network_io.c -o network_io

bibakBoxClient:
	gcc -g bibakBoxClient.c -o bibakBoxClient

bibakBoxServer:
	gcc -g bibakBoxServer.c -o bibakBoxServer

runClient:
	./bibakBoxClient ./clientDir 8080 10.211.55.13

runClientValgrind:
	valgrind  --leak-check=full -s ./bibakBoxClient ./clientDir 8080 10.211.55.13

runClientGDB:
	gdb --args ./bibakBoxClient ./clientDir 8080 10.211.55.13

runServer:
	./bibakBoxServer ./serverDir 10 8080

runServerValgrind:
	valgrind  --leak-check=full -s ./bibakBoxServer ./serverDir 10 8080

clean:
	rm bibakBoxClient bibakBoxServer file_io network_io
