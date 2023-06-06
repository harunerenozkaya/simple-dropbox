#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "file_io.c"

#define BUFFER_SIZE 1024
int count = 0;

void initialize_log_file(char* dir_name){
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));

    //Create log file path
    char* log_file_path = malloc(sizeof(dir_name) + sizeof("log.txt") + 3);
    sprintf(log_file_path,"%s/%s",dir_name,"log.txt");

    //Take the current directory info
    search_dir(dir_name,curr_dir_info);

    //Write log file
    write_log_file(log_file_path,curr_dir_info);

    free(curr_dir_info);
    free(log_file_path);
}

void control_local_changes(char* dir_name,int client_socket){
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
    dir_info_bibak log_dir_info;

    //Take the current directory info
    search_dir(dir_name,curr_dir_info);

    printf("a\n");

    //Take the directory info in the log file
    char* log_file_path = malloc(sizeof(dir_name) + sizeof("log.txt"));
    sprintf(log_file_path,"%s/%s",dir_name,"log.txt");
    printf("selam");
    log_dir_info = read_log_file(log_file_path);

    printf("b\n");

    printf("\n===============================\n");
    char* current_str = generate_dir_info_str(curr_dir_info);
    char* log_str = generate_dir_info_str(&log_dir_info);
    printf("Current : \n%s",current_str);
    printf("\nLog File : \n%s",log_str);
    printf("\n===============================\n");

    printf("c\n");
    
    free(curr_dir_info->files);
    free(log_dir_info.files);


    printf("d\n");
    free(curr_dir_info);

    free(current_str);
    free(log_str);
    free(log_file_path);

    printf("e\n");

    printf("\n%d\n",count++);
}

void controlRemoteChanges(char* dirName,int clientSocket){

}

int main(int argc, char* argv[]) {

    // Control if the argument count is okay
    if (argc < 3) {
        printf("Usage: %s [dirName] [portnumber]\n", argv[0]);
        return 1;
    }

    // Extract arguments
    char* dirName = argv[1];
    int portNumber = atoi(argv[2]);

    // Create a socket with IPV4 TCP protocol
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("ERROR : Socket has not been created!");
        return 1;
    }

    // Set up server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("ERROR : Couldn't be connected to server!");
        return 1;
    }

    // Success message
    printf("SUCCESS : Connection has been established with Server.\n");

    initialize_log_file(dirName);

    //Start main loop
    while(1){
        control_local_changes(dirName,clientSocket);
        controlRemoteChanges(dirName,clientSocket);
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
