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
        request* requests = NULL;
        int request_count = control_local_changes(dirName,clientSocket,&(requests));
        int bytesRead = 0;
        long file_size = 0;
        FILE *file;

        //Send to server
        for(int i = 0; i < request_count; i++){
            int buffer_size = 1024;
            char buffer[buffer_size];
            memset(buffer, 0, sizeof(buffer));

            //Delete local dir part
            extract_local_dir_path_part(&(requests[i]),dirName);

            //Convert request to json
            char* request_json = request_to_json(requests[i],buffer_size);
            
            //Copy json to buffer
            strcpy(buffer,request_json);

            //Send request to server
            send(clientSocket, buffer, sizeof(buffer), 0);

            free(request_json);

            //Re append local dir part
            append_local_dir_path_part(&(requests[i]) , dirName);

            //Handle request continue
            switch(requests[i].request_t){
                //UPLOAD
                case 0:
                    memset(buffer, 0, sizeof(buffer));

                    //TODO control if the file is null and notify server
                    file = fopen(requests[i].file.path,"rb");

                    bytesRead = 0;
                    //If there is content in the file
                    while ((bytesRead = fread(buffer, 1 , sizeof(buffer) , file)) > 0) {
                        send(clientSocket, buffer, bytesRead, 0);
                    }
                    
                    //Close the file
                    fclose(file);
                    
                    //Get response
                    read(clientSocket,buffer,sizeof(buffer));
                    printf("\nresponse : %s\n",buffer);

                    break;
                //DOWNLOAD
                case 1:
                    break;
                //DELETE
                case 2:
                    break;
                //UPDATE
                case 3:
                    memset(buffer, 0, sizeof(buffer));

                    //TODO control if the file is null and notify server
                    file = fopen(requests[i].file.path,"rb");

                    bytesRead = 0;
                    //If there is content in the file
                    while ((bytesRead = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
                        int n = write(clientSocket, buffer, bytesRead);
                        if (n < 0) {
                            perror("Error writing to socket");
                        }
                    }
                    
                    //Close the file
                    fclose(file);
                    
                    //Get response
                    read(clientSocket,buffer,sizeof(buffer));
                    printf("\nresponse : %s\n",buffer);

                    break;
            }
        }

        //Free requests memory
        for(int i = 0; i < request_count; i++){
            free(requests[i].file.name);
            free(requests[i].file.path);
        }
        free(requests);








        controlRemoteChanges(dirName,clientSocket);
        sleep(3);
    }

    // Close the client socket
    close(clientSocket);

    return 0;
}
