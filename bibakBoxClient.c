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


// Free memory
void free_client_log_allocated_memory(dir_info_bibak* curr_dir_info, dir_info_bibak* log_dir_info, char* log_file_path) {
    for (int i = 0; i < curr_dir_info->total_file_count; i++) {
        free(curr_dir_info->files[i].name);
        free(curr_dir_info->files[i].path);
    }
    
    if (curr_dir_info->total_file_count > 0)
        free(curr_dir_info->files);

    free(curr_dir_info);

    for (int i = 0; i < log_dir_info->total_file_count; i++) {
        free(log_dir_info->files[i].name);
        free(log_dir_info->files[i].path);
    }
    
    free(log_dir_info->files);
    free(log_dir_info);
    
    free(log_file_path);
}

//Free allocated memory
void free_server_client_dir_info_and_log(dir_info_bibak* server_dir_info, dir_info_bibak* client_dir_info, char* client_log_file_path, char* server_log) {
    for (int i = 0; i < server_dir_info->total_file_count; i++) {
        free(server_dir_info->files[i].name);
        free(server_dir_info->files[i].path);
    }
    
    if (server_dir_info->total_file_count > 0)
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
}

// Free memory of the given requests
void free_request_memory(request* requests, int request_count) {
    for (int i = 0; i < request_count; i++) {
        free(requests[i].file.name);
        free(requests[i].file.path);
    }
    free(requests);
}



//Initialize the log file as empty if it doesn't exist
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

//Compare log and current directory to indicate the differenciesies
int compare_log_and_current_dir(char* dir_name,dir_info_bibak* curr_dir_info, dir_info_bibak* log_dir_info ,request** requests){
    
    // Allocate dir memories
    dir_info_bibak* new_log_dir_info = malloc(sizeof(dir_info_bibak));
    new_log_dir_info->total_file_count = 0;
    new_log_dir_info->last_modified_time[0] = '\0';
    new_log_dir_info->files = NULL;

    int isFound = 0;
    int request_count = 0;

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

    //If there is a change
    if(request_count > 0){
        //Create log file path
        char* log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
        strcpy(log_file_path, dir_name);
        strcat(log_file_path, "/log.txt");

        //Update log file
        write_log_file(log_file_path,new_log_dir_info);
        
        free(log_file_path);
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

// Take current directory and log directory and send to comare function to compare
int control_local_changes(char* dir_name,int client_socket ,request** requests){

    // Allocate the directory memories
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
    
    //Compare the current dir structure and log file structure
    int request_count = compare_log_and_current_dir(dir_name,curr_dir_info,log_dir_info,requests);

    // Free allocated memory
    free_client_log_allocated_memory(curr_dir_info, log_dir_info, log_file_path);

    return request_count;
}

//Send the requests of local changes to server updates itself
void send_local_change_requests(int clientSocket, request* requests, int request_count, char* dirName) {
    for (int i = 0; i < request_count; i++) {

        //Prepare the buffer
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        //Remove the base part of the path to send server relative path
        extract_local_dir_path_part(&(requests[i]), dirName);

        //Send request to the server
        char* request_json = request_to_json(requests[i], BUFFER_SIZE);
        strcpy(buffer, request_json);
        free(request_json);

        send(clientSocket, buffer, sizeof(buffer), 0);

        //Add the base part of the path back
        append_local_dir_path_part(&(requests[i]), dirName);

        switch (requests[i].request_t) {
            
            // UPLOAD
            case 0: 
            { 
                // Open the file to be uploaded
                memset(buffer, 0, sizeof(buffer));
                FILE* file = fopen(requests[i].file.path, "rb");

                // Read the content of the file
                size_t bytesRead = 0;
                while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(clientSocket, buffer, bytesRead, 0);
                }

                //Close the file
                fclose(file);

                break;
            }

            // DELETE
            case 2: 
            {
                memset(buffer, 0, sizeof(buffer));
                break;
            }

            // UPDATE
            case 3: 
            {
                // Open the file to be updated 
                memset(buffer, 0, sizeof(buffer));
                FILE* file = fopen(requests[i].file.path, "rb");

                // Read the content of the file
                size_t bytesRead = 0;
                while ((bytesRead = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
                    int n = write(clientSocket, buffer, bytesRead);
                    if (n < 0) {
                        perror("Error writing to socket");
                    }
                }

                //Close the file
                fclose(file);
                break;
            }
        }

        //Get the response and print
        read(clientSocket, buffer, sizeof(buffer));
        printf("\nresponse: %s\n", buffer);

        memset(buffer, '\0', sizeof(buffer));
    }

}

// Send server LOG request to get server current directory
char* get_server_log(int clientSocket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //Prepare the request
    request req;
    req.request_t = LOG;
    req.file.name = NULL;
    req.file.size = 0;
    req.file.path = NULL;

    //Convert request to json
    char* request_json = request_to_json(req, BUFFER_SIZE);
    strcpy(buffer, request_json);
    free(request_json);

    //Send LOG request to server
    send(clientSocket, buffer, sizeof(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));

    //Get log length from server
    recv(clientSocket, buffer, sizeof(buffer), 0);
    int server_log_size = atoi(buffer);

    // Allocate server_log memory according to log file size
    char* server_log = malloc((server_log_size) * sizeof(char));
    memset(server_log, '\0', (server_log_size) * sizeof(char));
    memset(buffer, '\0', sizeof(buffer));

    //Read the log file content
    int willRead = 0;
    int bytesRead = 0;
    size_t start = 0;

    if (server_log_size - start < sizeof(buffer)) {
        willRead = server_log_size - start;
    }
    else {
        willRead = sizeof(buffer);
    }

    while (start < server_log_size && (bytesRead = recv(clientSocket, buffer, willRead, 0)) > 0) {
        strncpy(server_log + start, buffer, bytesRead);
        memset(buffer, '\0', sizeof(buffer));

        start += bytesRead;

        if (server_log_size - start < sizeof(buffer)) {
            willRead = server_log_size - start;
        }
        else {
            willRead = sizeof(buffer);
        }
    }

    server_log[server_log_size - 1] = '\0';

    //Get response
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("\nresponse: %s\n", buffer);

    return server_log;
}

//Compare server dir and client dir and indicate the differencies and generate requests
int compare_server_and_client_dir(dir_info_bibak* server_dir_info, dir_info_bibak* client_dir_info ,request** requests , char* local_dir){
    int request_count = 0;
    int isFound = 0;


    //Search server files in the client log file
    for(int i = 0; i < server_dir_info->total_file_count; i++){
        isFound = 0;

        for(int j = 0; j < client_dir_info->total_file_count; j++){
            
            //Extract the base part of the client dir path
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

// Control current directory and server direcytory to indicate differencieses
int control_remote_changes(char* dir_name,int clientSocket, request** requests){

    //Get server current directory
    char* server_log = get_server_log(clientSocket);

    //Parse server log file
    dir_info_bibak* server_dir_info = parse_dir_info_str(server_log);
    
    //Parse client log file
    char* client_log_file_path = malloc(strlen(dir_name) + strlen("/log.txt") + 1);
    strcpy(client_log_file_path, dir_name);
    strcat(client_log_file_path, "/log.txt");
    
    dir_info_bibak* client_dir_info = read_log_file(client_log_file_path);

    //Compare the directories
    int request_count = compare_server_and_client_dir(server_dir_info,client_dir_info,requests,dir_name);

    //Free allocated memories
    free_server_client_dir_info_and_log(server_dir_info, client_dir_info, client_log_file_path, server_log);


    return request_count;
}

//Send server download (fetch file from server) request or delete the file if the file doesn't exist in the server
void send_server_change_requests(int clientSocket, request* requests, int request_count, char* dirName) {
    for (int i = 0; i < request_count; i++) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        //Convert request to json
        char* request_json = request_to_json(requests[i], BUFFER_SIZE);
        strcpy(buffer, request_json);
        free(request_json);

        //Handle requests according to types
        switch (requests[i].request_t) {

            // DOWNLOAD
            case 1: 
                // Send request to the server
                send(clientSocket, buffer, sizeof(buffer), 0);
                memset(buffer, 0, sizeof(buffer));
                
                //Get server side file size
                int file_data_length = requests[i].file.size;

                //Control if the file is downloadable
                FILE* file_descriptor = create_file_and_dir(requests[i].file , dirName);

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
                //TODO if there is an error delete created inner directories
                
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

//After the synchorinizing client with server , update the log file for last changes
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

    //Initialize the log file as empty if it doesn't exist
    initialize_log_file(dirName);

    //Start main loop
    while(1){
        //Control current client directory and log file and detect the differencies
        request* requests = NULL;
        int request_count = control_local_changes(dirName,clientSocket,&(requests));

        // Send the differencies to the server
        send_local_change_requests(clientSocket,requests,request_count,dirName);

        // Free requests memory
        free_request_memory(requests,request_count);
      
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
        free_request_memory(requests,request_count);
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
