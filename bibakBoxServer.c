#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main(int argc, char* argv[]) {

    // Control if the argument count is okay
    if (argc < 4) {
        printf("Usage: %s [directory] [threadPoolSize] [portnumber]\n", argv[0]);
        return 1;
    }

    // Extract arguments
    const char* directory = argv[1];
    int threadPoolSize = atoi(argv[2]);
    int portNumber = atoi(argv[3]);

    // Open the directory
    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("ERROR : Directory has not been opened!");
        return 1;
    }
    closedir(dir);

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("ERROR : Socket has not been created!");
        return 1;
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        return 1;
    }

    printf("SUCCESS : Server has been established !\n");
    printf("Waitining for connections!\n");
    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("Error listening for connections");
        return 1;
    }

    // Accept client connection
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
    if (clientSocket < 0) {
        perror("Error accepting connection");
        return 1;
    }

    printf("SUCCESS : A client has been connected to server!\n");

    while(1){
        
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}
