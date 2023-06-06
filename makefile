all: file_io.o bibakBoxClient.o bibakBoxServer.o

file_io.o:
	gcc -c -g file_io.c -o file_io.o

bibakBoxClient.o:
	gcc bibakBoxClient.c -o bibakBoxClient.o

bibakBoxServer.o:
	gcc bibakBoxServer.c -o bibakBoxServer.o

runFileIO:
	./file_io.o

runClient:
	./bibakBoxClient.o ./clientDir 80

runServer:
	./bibakBoxServer.o ./serverDir 10 80

clean:
	rm *.o
