#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "network_io.c"

#define BUFFER_SIZE 1024
int count = 0;

void initialize_log_file(char* dir_name){
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
    curr_dir_info->total_file_count = 0;
    curr_dir_info->last_modified_time[0] = '\0';

    //Create log file path
    char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(log_file_path, dir_name);
    strcat(log_file_path, "/log.txt");

    //Take the current directory info
    search_dir(dir_name,curr_dir_info);

    //Write log file
    write_log_file(log_file_path,curr_dir_info);

    free(curr_dir_info);
    free(log_file_path);
}

request* compare_log_and_current_dir(dir_info_bibak* curr_dir_info, dir_info_bibak* log_dir_info){
    request* requests;

    if(curr_dir_info->total_file_count != log_dir_info->total_file_count && curr_dir_info->last_modified_time != log_dir_info->last_modified_time){
        for(int i = 0; i < curr_dir_info->total_file_count; i++){
            for(int j = 0; j < log_dir_info->total_file_count; j++){

            }
        }
        requests = malloc(0);
    }
    else{
        requests = malloc(0);
    }

    
    printf("count curr : %d\n",curr_dir_info->total_file_count);
    printf("count log : %d\n",log_dir_info->total_file_count);

    return requests;
}

void control_local_changes(char* dir_name,int client_socket){
    //TODO
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
    curr_dir_info->total_file_count = 0;
    curr_dir_info->last_modified_time[0] = '\0';

    dir_info_bibak* log_dir_info;

    // Take the current directory info
    search_dir(dir_name,curr_dir_info);

    // Take the directory info in the log file
    char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(log_file_path, dir_name);
    strcat(log_file_path, "/log.txt");

    // Read the log file and convert to info structure
    log_dir_info = read_log_file(log_file_path);

    // Print the current directory and log file directory
    printf("\n===============================\n");
    char* current_str = generate_dir_info_str(curr_dir_info);
    char* log_str = generate_dir_info_str(log_dir_info);
    printf("Current : \n%s",current_str);
    printf("\nLog File : \n%s",log_str);
    printf("\n===============================\n");


    request* requests = compare_log_and_current_dir(curr_dir_info,log_dir_info);


    // Free allocated memory
    free(requests);

    for (int i = 0; i < curr_dir_info->total_file_count; i++) {
        free(curr_dir_info->files[i].name);
        free(curr_dir_info->files[i].path);
    }

    //free(curr_dir_info->files);
    
    free(curr_dir_info);
    
    for (int i = 0; i < log_dir_info->total_file_count; i++) {
        //free(log_dir_info->files[i].name);
        //free(log_dir_info->files[i].path);
    }

    free(log_dir_info->files);

    free(current_str);
    free(log_str);
    free(log_file_path);

    count++;
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
        sleep(3);
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
