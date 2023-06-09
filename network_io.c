#include "file_io.c"

typedef enum {
    UPLOAD,
    DOWNLOAD,
    DELETE,
    UPDATE
} request_type;

typedef enum {
    DONE,
    ERROR
} response_status;

typedef struct {
    request_type request_t;
    file_bibak file;
} request;

typedef struct {
    response_status response_t;
    file_bibak file;
} response;

//Create a request with given parametes and assign to req
int create_request(request* req,request_type request_t, file_bibak file) {
    //copy request type
    req->request_t = request_t;

    //copy file name
    req->file.name = NULL;
    req->file.name = malloc(strlen(file.name) + 1);
    strcpy(req->file.name,file.name);

    //copy file path
    req->file.path = NULL;
    req->file.path = malloc(strlen(file.path) + 1);
    strcpy(req->file.path,file.path);

    //copy last_modified_time
    strcpy(req->file.last_modified_time,file.last_modified_time);

    //copy file size
    req->file.size = file.size;

    return 0;
}

//Create a response with given parametes and assign to res
int create_response(response* res,response_status response_t, file_bibak file) {
    //copy request type
    res->response_t = response_t;

    //copy file name
    res->file.name = NULL;
    res->file.name = malloc(strlen(file.name) + 1);
    strcpy(res->file.name,file.name);

    //copy file path
    res->file.path = NULL;
    res->file.path = malloc(strlen(file.path) + 1);
    strcpy(res->file.path,file.path);

    //copy last_modified_time
    strcpy(res->file.last_modified_time,file.last_modified_time);

    //copy file size
    res->file.size = file.size;

    return 0;
}

char* request_to_json(const request req ,int buffer_size) {
    char* json = malloc(sizeof(char)*buffer_size);

    sprintf(json, "{\"request_type\": %d, \"file_name\": \"%s\", \"file_last_modified_time\": \"%s\", \"file_size\": %d, \"file_path\": \"%s\"}",
            req.request_t, req.file.name, req.file.last_modified_time ,req.file.size, req.file.path);
    
    return json;
}

request* json_to_request(const char* json) {
    request* req = malloc(sizeof(request));

    int request_t;
    int file_size = 0;
    char request_type_str[10];
    char file_name[100];
    char file_last_modified_time[20];
    char file_size_str[10];
    char file_path[100];

    sscanf(json, "{\"request_type\": %d, \"file_name\": \"%[^\"]\", \"file_last_modified_time\": \"%[^\"]\", \"file_size\": %d, \"file_path\": \"%[^\"]\"}",
           &request_t, file_name, file_last_modified_time, &file_size, file_path);

    req->request_t = (request_type) request_t;

    req->file.name = malloc(strlen(file_name) + 1);
    strcpy(req->file.name,file_name);
    
    strncpy(req->file.last_modified_time,file_last_modified_time,sizeof(req->file.last_modified_time));
    req->file.size = file_size;

    req->file.path = malloc(strlen(file_path) + 1);
    strcpy(req->file.path,file_path);

    return req;
}

void extract_local_dir_path_part(request* req, char* local_dir) {
    // Calculate the length of the local directory
    int local_dir_len = strlen(local_dir);

    // Calculate the length of the remaining path
    int remaining_path_len = strlen(req->file.path) - local_dir_len;

    // Allocate memory for the relative path
    char* relative_path = (char*)malloc(remaining_path_len + 1);

    // Copy the remaining path to the relative path variable
    strcpy(relative_path, req->file.path + local_dir_len);

    // Update the file path in the request structure
    strcpy(req->file.path, relative_path);

    // Free the memory allocated for the relative path
    free(relative_path);
}

void append_local_dir_path_part(request* req, char* local_dir) {
    // Calculate the length of the local directory
    int local_dir_len = strlen(local_dir);

    // Calculate the length of the relative path
    int relative_path_len = strlen(req->file.path);

    // Calculate the total length of the updated file path
    int updated_file_path_len = local_dir_len + relative_path_len;

    // Allocate memory for the updated file path
    char* updated_file_path = (char*)malloc(updated_file_path_len + 1);

    // Copy the local directory to the updated file path
    strcpy(updated_file_path, local_dir);

    // Concatenate the relative path to the updated file path
    strcat(updated_file_path, req->file.path);

    // Update the file path in the request structure
    strcpy(req->file.path, updated_file_path);

    // Free the memory allocated for the updated file path
    free(updated_file_path);
}