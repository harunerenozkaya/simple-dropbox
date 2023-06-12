#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "network_io.c"
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

const char* directory;

// Create a file which will be uploaded from client and create directories if it is needed
FILE* upload_file(file_bibak file) {
    // Concatenate the directory and file path
    int total_length = strlen(directory) + strlen(file.path) + 1;
    char* file_path = (char*)malloc(total_length * sizeof(char));
    snprintf(file_path, total_length, "%s%s", directory, file.path);

    // Create a copy of the file path
    char* path_copy = strdup(file_path);

    // Find the last occurrence of '/' in the file path
    char* last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) {
        printf("Invalid file path: %s\n", file_path);
        free(path_copy);
        return NULL;
    }

    // Extract the directory path and file name
    *last_slash = '\0';  // Null-terminate the directory path
    const char* directory_path = path_copy;
    const char* file_name = last_slash + 1;

    // Create the directories if they don't exist
    struct stat st;
    if (stat(directory_path, &st) != 0) {
        // Directory does not exist, create it
        int result = mkdir(directory_path, 0777);
        if (result != 0) {
            printf("Error creating directory: %s\n", directory_path);
            free(path_copy);
            return NULL;
        }
    }

    // Check if the file already exists
    FILE* existing_file = fopen(file_path, "rb");
    if (existing_file != NULL) {
        fclose(existing_file);
        free(file_path);
        return NULL; // File with the same name already exists, return NULL as an error indicator
    }

    // Create the new file
    FILE* new_file = fopen(file_path, "wb");
    if (new_file == NULL) {
        free(file_path);
        return NULL; // Error creating the file, return NULL as an error indicator
    }

    //printf("File created successfully: %s\n", file_path);
    return new_file;
}

// Create a file which will be updated from client and create directories if it is needed
FILE* update_file(file_bibak file) {
    
    // Concatenate the directory and file path
    int total_length = strlen(directory) + strlen(file.path) + 1;
    char* result_path = (char*)malloc(total_length * sizeof(char));
    snprintf(result_path, total_length, "%s%s", directory, file.path);

    // Check if the file isn't exist
    FILE* file_t = fopen(result_path, "rb");
    if (file_t == NULL) {
        free(result_path);
        return NULL; // File with the same name doesn't exists, return NULL as an error indicator
    }
    fclose(file_t);

    // Truncate the file and write
    FILE* new_file = fopen(result_path, "wb");
    if (new_file == NULL) {
        free(result_path);
        return NULL; // Error creating the file, return NULL as an error indicator
    }

    free(result_path);
    return new_file;
}

// Delete the file
int delete_file(file_bibak file){
    // Concatenate the directory and file path
    int total_length = strlen(directory) + strlen(file.path) + 1;
    char* result_path = (char*)malloc(total_length * sizeof(char));
    snprintf(result_path, total_length, "%s%s", directory, file.path);

    // Check if the file isn't exist
    FILE* file_t = fopen(result_path, "rb");
    if (file_t == NULL) {
        free(result_path);
        return -1; // File with the same name doesn't exists, return -1 as an error indicator
    }
    fclose(file_t);

    if(remove(result_path) != 0){
        free(result_path);
        return -1;
    }

    free(result_path);
    return 0;
}

// Open the file which will be downloaded by client
FILE* download_file(file_bibak file) {
    // Concatenate the directory and file path
    int total_length = strlen(directory) + strlen(file.path) + 1;
    char* result_path = (char*)malloc(total_length * sizeof(char));
    snprintf(result_path, total_length, "%s%s", directory, file.path);

    // Check if the file is exist
    FILE* file_t = fopen(result_path, "rb");
    if (file_t == NULL) {
        free(result_path);
        return NULL; // File with the same name doesn't exists, return NULL as an error indicator
    }
  
    free(result_path);
    return file_t;
}

// Handler for a client requests
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
        char buffer[1024];
        memset(buffer, '\0', sizeof(buffer));
        int n = read(client_socket, buffer, sizeof(buffer));
        if (n < 0) {
            perror("Error reading from socket");
            exit(1);
        }
        
        //Convert json to request structure
        request* req = json_to_request(buffer);

        //Print request details
        if(req->request_t >= 0 && req->request_t < 4){
            printf("\n===============================\n");
            printf("======= Request Received ======");
            printf("\n===============================\n\n");
            printf("request_type : %s\n",req->request_t == 0 ? "UPLOAD" : 
            req->request_t == 1 ? "DOWNLOAD" : 
            req->request_t == 2 ? "DELETE" : 
            "UPDATE");
            printf("file_name : %s\n",req->file.last_modified_time);
            printf("file_last_modified_time: %s\n",req->file.last_modified_time);
            printf("file_size:: %d\n",req->file.size);
            printf("file_path: %s\n",req->file.path);
            printf("\n===============================\n");
        }
        
        //Initialize request handling variables
        int bytesRead = 0;
        int writedByte = 0;
        int file_data_length = 0;

        response* res = malloc(sizeof(response));
        res->response_t = 0;
        res->file.name = NULL;
        res->file.path = NULL;
        res->file.size = 0;

        FILE* file_descriptor;
        char* json = NULL;

        int is_valid = 0;

        switch(req->request_t){
            //UPLOAD
            case 0:

                //Get File size
                memset(buffer, '\0', sizeof(buffer));
                file_data_length = req->file.size;
                memset(buffer, 0, sizeof(buffer));

                //printf("size:%d\n",file_data_length);

                //Control if the file is uploadable
                file_descriptor = upload_file(req->file);

                //Get the file content         
                bytesRead = 0;
                writedByte = 0;
                while (writedByte < file_data_length && (bytesRead = recv(client_socket, buffer, sizeof(buffer) , 0)) > 0) {
                    //Write if the file is uploadable

                    writedByte += bytesRead;

                    if(file_descriptor != NULL){
                        fwrite(buffer,bytesRead,1,file_descriptor);
                    }
                }

                //Close the fd
                if(file_descriptor != NULL){
                    is_valid = 1;
                    fclose(file_descriptor);
                }

                //Change the new file last modification time to original last modification time
                if(is_valid == 1){
                    change_last_modification_time(req->file.path,directory,req->file.last_modified_time);
                }
                
                //Change response status if fd is NULL
                if(is_valid == 0){
                    res->response_t = 1;
                }
                break;
                
            //DOWNLOAD
            case 1:
                //Get the file
                memset(buffer, 0, sizeof(buffer));
                file_descriptor = download_file(req->file);
                
                //Read the file content and write to buffer
                bytesRead = 0;
                while (file_descriptor != NULL && (bytesRead = fread(buffer, sizeof(char), sizeof(buffer), file_descriptor)) > 0) {
                    int n = write(client_socket, buffer, bytesRead);
                    if (n < 0) {
                        perror("Error writing to socket");
                    }
                }

                //If server couldn't find the file write empty message
                if(file_descriptor == NULL){
                    int written = 0;
        
                    while(written < req->file.size){
                        memset(buffer,'\0',sizeof(buffer));
                        int n = write(client_socket, buffer, bytesRead);
                        written += n;
                    }
                }

                //Close the fd
                if(file_descriptor != NULL){
                    fclose(file_descriptor);
                    is_valid = 1;
                }

                //Change response status if fd is NULL
                if(is_valid == 0){
                    res->response_t = 1;
                }

                break;

            //DELETE
            case 2:
                //Delete the file
                int is_deleted = delete_file(req->file);

                if(is_deleted != 0){
                    res->response_t = 1;
                }

                break;

            //UPDATE
            case 3:
                //Get File size
                memset(buffer, 0, sizeof(buffer));
                file_data_length = req->file.size;

                //printf("size:%d\n",file_data_length);

                //Control if the file is uploadable
                file_descriptor = update_file(req->file);

                //Get the file content         
                bytesRead = 0;
                writedByte = 0;
                while (writedByte < file_data_length && (bytesRead = recv(client_socket, buffer, sizeof(buffer),0)) > 0) {
                    //Write if the file is uploadable
                    writedByte += bytesRead;

                    if(file_descriptor != NULL){
                        fwrite(buffer,bytesRead,1,file_descriptor);
                    }
                }

                //Close the fd
                if(file_descriptor != NULL){
                    is_valid = 1;
                    fclose(file_descriptor);
                }

                //Change the new file last modification time to original last modification time
                if(is_valid == 1){
                    change_last_modification_time(req->file.path,directory,req->file.last_modified_time);
                }
                
                //Change response status if fd is NULL
                if(is_valid == 0){
                    res->response_t = 1;
                }

                break;

            //LOG
            case 4:
                // Allocate the memory
                dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
                curr_dir_info->total_file_count = 0;
                curr_dir_info->last_modified_time[0] = '\0';
                curr_dir_info->files = NULL;

                // Search the directory of the server
                search_dir(directory,curr_dir_info);    

                // Convert server directory to the json to send
                char* str = generate_dir_info_str(curr_dir_info,1,directory);
                int length = strlen(str);

                //Send to client the log file length
                memset(buffer, '\0', sizeof(buffer));
                sprintf(buffer,"%d",length);
                send(client_socket,buffer,sizeof(buffer),0);

                // Send server current structure
                size_t start = 0;
                while (start < length) {
                    memset(buffer, '\0', sizeof(buffer));
                    size_t chunk_size = (length - start < sizeof(buffer)) ? length - start : sizeof(buffer);
                    strncpy(buffer, str + start, chunk_size);

                    if(write(client_socket,buffer,chunk_size) < 0){
                        perror("ERROR : Response can not be sent to the server!\n");
                    }

                    start += chunk_size;
                }
                
                //Free allocated memories
                free(str);
                
                for (int i = 0; i < curr_dir_info->total_file_count; i++) {
                    free(curr_dir_info->files[i].name);
                    free(curr_dir_info->files[i].path);
                }
                
                if(curr_dir_info->total_file_count > 0)
                    free(curr_dir_info->files);

                free(curr_dir_info);
                break;

        }

        memset(buffer, 0, sizeof(buffer));
    
        //Prepare the response
        json = response_to_json(res,sizeof(buffer));

        //Send the response
        strcpy(buffer,json);
        if(write(client_socket,buffer,sizeof(buffer)) < 0){
            perror("ERROR : Response can not be sent to the server!\n");
        }

        // Print response message
        if(req->request_t >= 0 && req->request_t < 4){
            printf("\n===============================\n");
            printf("======= Response Sent ======");
            printf("\n===============================\n\n");
            printf("response_type : %s\n\n",res->response_t == 0 ? "DONE" : "ERROR");
        }

        free(json);
        free(req->file.name);
        free(req->file.path);
        free(req);
        free(res->file.name);
        free(res->file.path);
        free(res);
        memset(buffer, '\0', sizeof(buffer));
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
    directory = argv[1];
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
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portNumber);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

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
    printf("Server listening on port %d\n",portNumber);

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
