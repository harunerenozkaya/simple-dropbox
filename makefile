all: clean file_io.o network_io.o bibakBoxClient.o bibakBoxServer.o 

file_io.o:
	gcc -c -g file_io.c -o file_io.o

network_io.o:
	gcc -c -g network_io.c -o network_io.o

bibakBoxClient.o:
	gcc -g bibakBoxClient.c -o bibakBoxClient.o

bibakBoxServer.o:
	gcc -g bibakBoxServer.c -o bibakBoxServer.o

runFileIO:
	./file_io.o

runClient:
	./bibakBoxClient.o ./clientDir 8080

runClientValgrind:
	valgrind  --leak-check=full -s ./bibakBoxClient.o ./clientDir 8080 

runClientGDB:
	gdb --args ./bibakBoxClient.o ./clientDir 8080

runServer:
	./bibakBoxServer.o ./serverDir 10 8080

clean:
	rm *.o
