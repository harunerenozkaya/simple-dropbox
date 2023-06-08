#include "file_io.c"

typedef enum {
    UPLOAD,
    DOWNLOAD,
    DELETE,
    UPDATE
} request_type;

typedef struct {
    request_type request_t;
    file_bibak file;
} request;

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