#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "network_io.c"

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

void *handle_client(void *arg) {
    int server_socket = *(int *)arg;

    // Accept a new client connection
    int client_socket = accept(server_socket, NULL, NULL);
    if (client_socket < 0) {
        perror("Error accepting client connection");
        exit(1);
    }

    while (1) {
        // Read data from the client
        char buffer[1000];
        memset(buffer, 0, sizeof(buffer));
        int n = read(client_socket, buffer, sizeof(buffer));
        if (n < 0) {
            perror("Error reading from socket");
            exit(1);
        }

        request* req = json_to_request(buffer);

        printf("\n===============================\n");
        printf("======= Request Received ======");
        printf("\n===============================\n\n");
        printf("request_type : %d\n",req->request_t);
        printf("file_name : %s\n",req->file.last_modified_time);
        printf("file_last_modified_time: %s\n",req->file.last_modified_time);
        printf("file_size:: %d\n",req->file.size);
        printf("file_path: %s\n",req->file.path);
        printf("\n===============================\n");



        // Process the data (echo back in this case)
        //printf("\nReceived from client: %s\n", buffer);
        if (write(client_socket, buffer, n) < 0) {
            perror("Error writing to socket");
            exit(1);
        }

        free(req->file.name);
        free(req->file.path);
        free(req);
    }

    close(client_socket);
}

int main(int argc, char* argv[]) {

    // Control if the argument count is okay
    if (argc < 4) {
        printf("Usage: %s [directory] [thread_pool_size] [portnumber]\n", argv[0]);
        return 1;
    }

    // Extract arguments
    const char* directory = argv[1];
    int thread_pool_size = atoi(argv[2]);
    int portNumber = atoi(argv[3]);

    // Open the directory
    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("ERROR : Directory has not been opened!");
        return 1;
    }
    closedir(dir);

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("ERROR : Socket has not been created!");
        return 1;
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, thread_pool_size) < 0) {
        perror("Error listening for connections");
        return 1;
    }

    printf("SUCCESS : Server has been established !\n");
    printf("Server listening on port 8888\n");

    // Create a thread pool
    pthread_t thread_pool[thread_pool_size];

    for (int i = 0; i < thread_pool_size; i++) {
        if (pthread_create(&thread_pool[i], NULL, handle_client, &server_socket) != 0) {
            perror("Error creating thread");
            return 1;
        }
    }

    // Join all the threads
    for (int i = 0; i < thread_pool_size; i++) {
        pthread_join(thread_pool[i], NULL);
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
