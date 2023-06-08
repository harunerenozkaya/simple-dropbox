#include "file_io.c"

typedef enum {
    UPLOAD,
    DOWNLOAD,
    DELETE,
    ASYNC
} request_type;

typedef struct {
    request_type request_t;
    file_bibak file;
} request;

request create_request(request_type request_t, file_bibak file) {
    request req;
    req.request_t = request_t;
    req.file = file;
    return req;
}