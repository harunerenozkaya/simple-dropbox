#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "network_io.c"
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVER_IP_ADRESS "192.168.1.115"
int count = 0;

FILE* download_file(file_bibak file , char* dir_name) {
    // Concatenate the directory and file path
    int total_length = strlen(dir_name) + strlen(file.path) + 1;
    char* file_path = (char*)malloc(total_length * sizeof(char));
    snprintf(file_path, total_length, "%s%s", dir_name, file.path);

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

    // Create the new file
    FILE* new_file = fopen(file_path, "wb");
    if (new_file == NULL) {
        free(file_path);
        return NULL; // Error creating the file, return NULL as an error indicator
    }

    //printf("File created successfully: %s\n", file_path);
    return new_file;
}

void initialize_log_file(char* dir_name){
    //Create log file path
    char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(log_file_path, dir_name);
    strcat(log_file_path, "/log.txt");

    FILE *fp;
    if ((fp = fopen(log_file_path,"r")) == NULL) {
        //If log file doesn't exist then create
        dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
        curr_dir_info->total_file_count = 0;
        curr_dir_info->last_modified_time[0] = '\0';
        curr_dir_info->files = malloc(0);

        //Write log file
        write_log_file(log_file_path,curr_dir_info);

        free(curr_dir_info);
    }
    else
        fclose(fp);
    
    free(log_file_path);
}

int compare_log_and_current_dir(char* dir_name,dir_info_bibak* curr_dir_info, dir_info_bibak* log_dir_info ,request** requests){
    int request_count = 0;
   
    dir_info_bibak* new_log_dir_info = malloc(sizeof(dir_info_bibak));
    new_log_dir_info->total_file_count = 0;
    new_log_dir_info->last_modified_time[0] = '\0';
    new_log_dir_info->files = NULL;

    int isFound = 0;

    /*
    //Print log and current dir main attributes
    printf("curr_dir_info->total_file_count : %d\n",curr_dir_info->total_file_count);
    printf("log_dir_info->total_file_count  : %d\n",log_dir_info->total_file_count );
    printf("curr_dir_info->last_modified_time  : %s\n",curr_dir_info->last_modified_time);
    printf("log_dir_info->last_modified_time  : %s\n",log_dir_info->last_modified_time);
    */

    
    //Search current files in the log file
    for(int i = 0; i < curr_dir_info->total_file_count; i++){
        isFound = 0;
        for(int j = 0; j < log_dir_info->total_file_count; j++){
            if( strcmp(curr_dir_info->files[i].name , log_dir_info->files[j].name) == 0 && 
                strcmp(curr_dir_info->files[i].path , log_dir_info->files[j].path) == 0){
                //File is found.
                isFound = 1;

                if(strcmp(curr_dir_info->files[i].last_modified_time,log_dir_info->files[j].last_modified_time) != 0){
                    // The last modified time is different of log and current file so it has modified.
                    // Post request to server to the update the file
                    printf("UPDATE : %s\n",curr_dir_info->files[i].name);

                    //Prepare request
                    request_count += 1;
                    *requests = realloc(*requests,request_count * sizeof(request));
                    create_request(&(*requests)[request_count - 1], UPDATE, curr_dir_info->files[i]);
                }

                //Add new dir_info to write the log file
                add_file_to_dir(new_log_dir_info,curr_dir_info->files[i]);
                break;
            }
        }

        if(isFound == 0){
            // File is added new , post request to server the upload the file
            printf("UPLOAD : %s\n",curr_dir_info->files[i].name);

            //Prepare request
            request_count += 1;
            *requests = realloc(*requests,request_count * sizeof(request));
            create_request(&(*requests)[request_count - 1], UPLOAD, curr_dir_info->files[i]);
            
            //Add new dir_info to write the log file
            add_file_to_dir(new_log_dir_info,curr_dir_info->files[i]);
        }
    }

    //Search log files in current files to determine deleted files
    for(int i = 0; i < log_dir_info->total_file_count; i++){
        isFound = 0;
        for(int j = 0; j < curr_dir_info->total_file_count; j++){
            if( strcmp(log_dir_info->files[i].name , curr_dir_info->files[j].name) == 0 && 
                strcmp(log_dir_info->files[i].path , curr_dir_info->files[j].path) == 0){
                //File is found.
                isFound = 1;
                break;
            }
        }

        if(isFound == 0){
            //TODO change deleted file modified date as deleted time

            // File is deleted , post request to server the delete the file
            printf("DELETE : %s\n",log_dir_info->files[i].name);

            //Prepare request
            request_count += 1;
            *requests = realloc(*requests,request_count * sizeof(request));
            create_request(&(*requests)[request_count - 1], DELETE, log_dir_info->files[i]);
        
        }
    }

    //If there is change
    if(request_count > 0){
        //Create log file path
        char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
        strcpy(log_file_path, dir_name);
        strcat(log_file_path, "/log.txt");

        //Update log file
        write_log_file(log_file_path,new_log_dir_info);
        
        free(log_file_path);
    }
    else{
        //printf("There is no change!");
    }

    //Free new log file struct allocated memory
    for (int i = 0; i < new_log_dir_info->total_file_count; i++) {
        free(new_log_dir_info->files[i].name);
        free(new_log_dir_info->files[i].path);
    }

    free(new_log_dir_info->files);
    free(new_log_dir_info);

    return request_count;
}

int control_local_changes(char* dir_name,int client_socket ,request** requests){
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
    curr_dir_info->total_file_count = 0;
    curr_dir_info->last_modified_time[0] = '\0';
    curr_dir_info->files = NULL;

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
    /*
    printf("\n===============================\n");
    char* current_str = generate_dir_info_str(curr_dir_info);
    char* log_str = generate_dir_info_str(log_dir_info);
    printf("Current : \n%s",current_str);
    printf("\nLog File : \n%s",log_str);
    printf("\n===============================\n");
    free(current_str);
    free(log_str);
    */
    
    //printf("\n===============================\n");
    int request_count = compare_log_and_current_dir(dir_name,curr_dir_info,log_dir_info,requests);
    //printf("\n===============================\n");
    

    // Free allocated memory

    for (int i = 0; i < curr_dir_info->total_file_count; i++) {
        free(curr_dir_info->files[i].name);
        free(curr_dir_info->files[i].path);
    }
    
    if(curr_dir_info->total_file_count > 0)
        free(curr_dir_info->files);

    free(curr_dir_info);

    for (int i = 0; i < log_dir_info->total_file_count; i++) {
        free(log_dir_info->files[i].name);
        free(log_dir_info->files[i].path);
    }
    
    free(log_dir_info->files);
    
    free(log_dir_info);
    free(log_file_path);
    
    count++;

    return request_count;
}

void send_local_change_requests(int clientSocket, request* requests, int request_count, char* dirName) {
    for (int i = 0; i < request_count; i++) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        extract_local_dir_path_part(&(requests[i]), dirName);

        char* request_json = request_to_json(requests[i], BUFFER_SIZE);
        strcpy(buffer, request_json);
        free(request_json);

        send(clientSocket, buffer, sizeof(buffer), 0);

        append_local_dir_path_part(&(requests[i]), dirName);

        switch (requests[i].request_t) {
            case 0: { // UPLOAD
                memset(buffer, 0, sizeof(buffer));
                FILE* file = fopen(requests[i].file.path, "rb");

                size_t bytesRead = 0;
                while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(clientSocket, buffer, bytesRead, 0);
                }

                fclose(file);

                read(clientSocket, buffer, sizeof(buffer));
                printf("\nresponse: %s\n", buffer);
                break;
            }
            case 2: { // DELETE
                memset(buffer, 0, sizeof(buffer));
                read(clientSocket, buffer, sizeof(buffer));
                printf("\nresponse: %s\n", buffer);
                break;
            }
            case 3: { // UPDATE
                memset(buffer, 0, sizeof(buffer));
                FILE* file = fopen(requests[i].file.path, "rb");

                size_t bytesRead = 0;
                while ((bytesRead = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
                    int n = write(clientSocket, buffer, bytesRead);
                    if (n < 0) {
                        perror("Error writing to socket");
                    }
                }

                fclose(file);

                read(clientSocket, buffer, sizeof(buffer));
                printf("\nresponse: %s\n", buffer);
                break;
            }
        }

        memset(buffer, '\0', sizeof(buffer));
    }

}

int compare_server_and_client_dir(dir_info_bibak* server_dir_info, dir_info_bibak* client_dir_info ,request** requests , char* local_dir){
    int request_count = 0;
    int isFound = 0;
    
    
    //Print log and current dir main attributes
    /*
    printf("server_dir_info->total_file_count : %d\n",server_dir_info->total_file_count);
    printf("client_dir_info->total_file_count  : %d\n",client_dir_info->total_file_count );
    printf("server_dir_info->last_modified_time  : %s\n",server_dir_info->last_modified_time);
    printf("client_dir_info->last_modified_time  : %s\n",client_dir_info->last_modified_time);

    printf("server : %s\n",generate_dir_info_str(server_dir_info,0,""));
    printf("client : %s\n",generate_dir_info_str(client_dir_info,0,""));
    */

    //Search server files in the client log file
    for(int i = 0; i < server_dir_info->total_file_count; i++){
        isFound = 0;

        for(int j = 0; j < client_dir_info->total_file_count; j++){
            
            char* client_relative_path = extract_local_dir_path_part_str(client_dir_info->files[j].path,local_dir);
        

            if( strcmp(server_dir_info->files[i].name , client_dir_info->files[j].name) == 0 && 
                strcmp(server_dir_info->files[i].path , client_relative_path) == 0){

                //File is found.
                isFound = 1;

                if(strcmp(server_dir_info->files[i].last_modified_time,client_dir_info->files[j].last_modified_time) != 0){
                    // The last modified time is different of server and client file so it has modified.
                    // Post request to server to the update the file
                    printf("DOWNLOAD : %s\n",server_dir_info->files[i].name);

                    //Prepare request
                    request_count += 1;
                    *requests = realloc(*requests,request_count * sizeof(request));
                    create_request(&(*requests)[request_count - 1], DOWNLOAD, server_dir_info->files[i]);

                }

                break;
            }
        }

        if(isFound == 0){
            // File is added new , post request to server the upload the file
            printf("DOWNLOAD : %s\n",server_dir_info->files[i].name);

            //Prepare request
            request_count += 1;
            *requests = realloc(*requests,request_count * sizeof(request));
            create_request(&(*requests)[request_count - 1], DOWNLOAD, server_dir_info->files[i]);
            
        }
    }

    //Search log files in current files to determine deleted files
    for(int i = 0; i < client_dir_info->total_file_count; i++){
        char* client_relative_path = extract_local_dir_path_part_str(client_dir_info->files[i].path,local_dir);

        isFound = 0;
        for(int j = 0; j < server_dir_info->total_file_count; j++){
            
            if( strcmp(client_dir_info->files[i].name , server_dir_info->files[j].name) == 0 && 
                strcmp(client_relative_path, server_dir_info->files[j].path) == 0){
                //File is found.
                isFound = 1;
                break;
            }
        }

        if(isFound == 0){
            // File is deleted , post request to server the delete the file
            printf("DELETE : %s\n",client_dir_info->files[i].name);

            //Prepare request
            request_count += 1;
            *requests = realloc(*requests,request_count * sizeof(request));
            create_request(&(*requests)[request_count - 1], DELETE, client_dir_info->files[i]);
        
        }
    }


    return request_count;
}

int control_remote_changes(char* dir_name,int clientSocket, request** requests){

    //Create buffer
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //Create LOG request
    request req;
    req.request_t = LOG;
    req.file.name = NULL;
    req.file.size = 0;
    req.file.path = NULL;

    //Create LOG request json
    char* request_json = request_to_json(req, BUFFER_SIZE);
    strcpy(buffer, request_json);
    free(request_json);

    //Send LOG request to server
    send(clientSocket, buffer, sizeof(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));

    //Get log file length
    int server_log_size = 0;
    recv(clientSocket, buffer, sizeof(buffer), 0);
    server_log_size = atoi(buffer);

    //printf("Log file length :%d\n",server_log_size);

    //Read the server log file
    char* server_log = malloc((server_log_size) * sizeof(char));
    memset(server_log , '\0' , sizeof(server_log));
    memset(buffer, '\0', sizeof(buffer));
   
    int willRead = 0;
    int bytesRead = 0;
    size_t start = 0;

    if(server_log_size - start < sizeof(buffer)){
        willRead = server_log_size - start;
    }
    else{
        willRead = sizeof(buffer);
    }

    while (start < server_log_size && (bytesRead = recv(clientSocket, buffer, willRead ,0)) > 0) {
        strncpy(server_log + start ,buffer, bytesRead);
        memset(buffer, '\0', sizeof(buffer));
    
        start += bytesRead;

    
        if(server_log_size - start < sizeof(buffer)){
            willRead = server_log_size - start;
        }
        else{
            willRead = sizeof(buffer);
        }

    }

    server_log[server_log_size-1] = '\0';
    //printf("\nserverlog : %s\n",server_log);

    //Get response
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("\nresponse : %s\n",buffer);

    //Parse server log file
    dir_info_bibak* server_dir_info = parse_dir_info_str(server_log);
    
    //Parse client log file
    char* client_log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(client_log_file_path, dir_name);
    strcat(client_log_file_path, "/log.txt");
    
    dir_info_bibak* client_dir_info = read_log_file(client_log_file_path);

    //Compare the directories
    int request_count = compare_server_and_client_dir(server_dir_info,client_dir_info,requests,dir_name);

    //free(req);
    for (int i = 0; i < server_dir_info->total_file_count; i++) {
        free(server_dir_info->files[i].name);
        free(server_dir_info->files[i].path);
    }
    
    if(server_dir_info->total_file_count > 0)
        free(server_dir_info->files);

    free(server_dir_info);

    for (int i = 0; i < client_dir_info->total_file_count; i++) {
        free(client_dir_info->files[i].name);
        free(client_dir_info->files[i].path);
    }
    
    free(client_dir_info->files);
    
    free(client_dir_info);

    free(client_log_file_path);
    free(server_log);

    return request_count;
}

void send_server_change_requests(int clientSocket, request* requests, int request_count, char* dirName) {
    for (int i = 0; i < request_count; i++) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        char* request_json = request_to_json(requests[i], BUFFER_SIZE);
        strcpy(buffer, request_json);
        free(request_json);

        switch (requests[i].request_t) {

            //TODO if there is an error delete created inner directories
            // DOWNLOAD
            case 1: 
                // Send request to the server
                send(clientSocket, buffer, sizeof(buffer), 0);
                memset(buffer, 0, sizeof(buffer));
                
                //Get server side file size
                int file_data_length = requests[i].file.size;

                //Control if the file is downloadable
                FILE* file_descriptor = download_file(requests[i].file , dirName);

                //Read file data
                int bytesRead = 0;
                int writedByte = 0;
                while (file_descriptor != NULL && writedByte < file_data_length && (bytesRead = recv(clientSocket, buffer, sizeof(buffer),0)) > 0) {
                    //Write if the file is uploadable
                    writedByte += bytesRead;

                    if(file_descriptor != NULL){
                        fwrite(buffer,bytesRead,1,file_descriptor);
                    }
                }

                //If file can not be opened properly
                while(file_descriptor == NULL && writedByte < file_data_length && (bytesRead = recv(clientSocket, buffer, sizeof(buffer),0)) > 0){
                    writedByte += bytesRead;
                }

                int is_valid = 0;

                //Close the fd
                if(file_descriptor != NULL){
                    is_valid = 1;
                    fclose(file_descriptor);
                }

                //Change the new file last modification time to original last modification time
                if(is_valid == 1){
                    change_last_modification_time(requests[i].file.path,dirName,requests[i].file.last_modified_time);
                }

                //Response
                read(clientSocket, buffer, sizeof(buffer));
                printf("\nresponse: %s\n", buffer);

                //If response status is error so rollback the operation
                
                break;

            // DELETE    
            case 2: { 
                remove(requests[i].file.path);
                break;
            }
        }

        memset(buffer, '\0', sizeof(buffer));
    }

}

int update_log_file(char* dir_name){
    dir_info_bibak* curr_dir_info = malloc(sizeof(dir_info_bibak));
    curr_dir_info->total_file_count = 0;
    curr_dir_info->last_modified_time[0] = '\0';
    curr_dir_info->files = NULL;

    // Take the current directory info
    search_dir(dir_name,curr_dir_info);

    //Create log file path
    char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(log_file_path, dir_name);
    strcat(log_file_path, "/log.txt");

    //Update log file
    write_log_file(log_file_path,curr_dir_info);
    
    // Free allocated memory
    for (int i = 0; i < curr_dir_info->total_file_count; i++) {
        free(curr_dir_info->files[i].name);
        free(curr_dir_info->files[i].path);
    }
    
    if(curr_dir_info->total_file_count > 0)
        free(curr_dir_info->files);

    free(curr_dir_info);

    free(log_file_path);

    return 0;
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
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADRESS);

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
        //Control current client directory and log file and detect the differencies
        request* requests = NULL;
        int request_count = control_local_changes(dirName,clientSocket,&(requests));

        // Send the differencies to the server
        send_local_change_requests(clientSocket,requests,request_count,dirName);

        // Free requests memory
        for (int i = 0; i < request_count; i++) {
            free(requests[i].file.name);
            free(requests[i].file.path);
        }
        free(requests);


        //Control the client directory and server directory and detect the differencies        
        requests = NULL;
        request_count = control_remote_changes(dirName,clientSocket,&(requests));

        // Send differencies to the server
        send_server_change_requests(clientSocket,requests,request_count,dirName);

        //Update the log file according to new remote changes
        if(request_count > 0){
            update_log_file(dirName);
        }

        // Free requests memory
        for (int i = 0; i < request_count; i++) {
            free(requests[i].file.name);
            free(requests[i].file.path);
        }
        free(requests);

    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
