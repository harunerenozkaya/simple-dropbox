all:
	gcc ./fileIO.c -o fileIO.o
	gcc ./bibakBoxClient.c -o bibakBoxClient.o
	gcc ./bibakBoxServer.c -o bibakBoxServer.o

runFileIO:
	./fileIO.o
	
runClient : 
	./bibakBoxClient.o ./clientDir 80

runServer :
	./bibakBoxServer.o ./serverDir 10 80

clean:
	rm *.o