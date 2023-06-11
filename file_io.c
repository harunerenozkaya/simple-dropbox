#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

typedef struct {
    char* name;
    char last_modified_time[20];
    int size;
    char* path;
} file_bibak;

typedef struct {
    int total_file_count;
    char last_modified_time[20];
    file_bibak* files;
} dir_info_bibak;

int parse_date_time(const char *date_time_str, struct tm *tm_struct) {
    int year, month, day, hour, minute, second;
    if (sscanf(date_time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        fprintf(stderr, "Failed to parse the date and time string\n");
        return -1;
    }

    tm_struct->tm_year = year - 1900;
    tm_struct->tm_mon = month - 1;
    tm_struct->tm_mday = day;
    tm_struct->tm_hour = hour;
    tm_struct->tm_min = minute;
    tm_struct->tm_sec = second;


    return 0;
}

int change_last_modification_time(FILE *file, const char *last_modification_time) {
    int fd = fileno(file);
    if (fd == -1) {
        perror("Failed to get file descriptor");
        return -1;
    }

    struct tm mod_time_struct;
    if (parse_date_time(last_modification_time, &mod_time_struct) == -1) {
        return -1;
    }

    printf("hour :%d\n",mod_time_struct.tm_hour);
    printf("minute :%d\n",mod_time_struct.tm_min);
    printf("second :%d\n",mod_time_struct.tm_sec);
    printf("is :%d\n",mod_time_struct.tm_isdst);

    struct timespec times[2];
    times[0].tv_sec = 0;  // Set access time to 0 to leave it unchanged
    times[0].tv_nsec = 0;
    times[1].tv_sec = mktime(&mod_time_struct);  // Set modification time
    times[1].tv_nsec = 0;

    if (futimens(fd, times) == -1) {
        perror("Failed to set file times");
        return -1;
    }

    printf("File times modified successfully\n");
    return 0;
}


char* read_file_data(const char* path) {
    FILE* file = fopen(path, "rb"); // Open the file in binary mode for reading
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END); // Move the file pointer to the end of the file
    long file_size = ftell(file); // Get the file size
    fseek(file, 0, SEEK_SET); // Move the file pointer back to the beginning of the file

    char* file_data = (char*)malloc(file_size + 1); // Allocate memory for the file data
    if (file_data == NULL) {
        fclose(file);
        perror("Error allocating memory for file data");
        return NULL;
    }

    size_t read_size = fread(file_data, 1, file_size, file); // Read the file data into the allocated memory
    if (read_size != file_size) {
        fclose(file);
        free(file_data);
        perror("Error reading file");
        return NULL;
    }

    file_data[file_size] = '\0'; // Null-terminate the file data

    fclose(file); // Close the file
    return file_data;
}

void buffer_reallocation(char* result,int buffer_size){
    if(strlen(result) + 200 > buffer_size){
        char* temp = realloc(result, buffer_size*2);
        if (temp == NULL) {
            fprintf(stderr, "Memory reallocation failed!\n");
            free(result);
        }
        strcpy(temp,result);
        free(result);
        result = temp;
    }
}

// Compare two time and return latest one
const char* get_latest_timestamp(const char* timestamp1, const char* timestamp2) {
    if (strlen(timestamp1) == 0)
        return timestamp2;
    if (strlen(timestamp2) == 0)
        return timestamp1;

    int year1, month1, day1, hour1, minute1, second1;
    int year2, month2, day2, hour2, minute2, second2;

    sscanf(timestamp1, "%d-%d-%d %d:%d:%d", &year1, &month1, &day1, &hour1, &minute1, &second1);
    sscanf(timestamp2, "%d-%d-%d %d:%d:%d", &year2, &month2, &day2, &hour2, &minute2, &second2);


    if (year1 > year2)
        return timestamp1;
    else if (year2 > year1)
        return timestamp2;

    if (month1 > month2)
        return timestamp1;
    else if (month2 > month1)
        return timestamp2;

    if (day1 > day2)
        return timestamp1;
    else if (day2 > day1)
        return timestamp2;

    if (hour1 > hour2)
        return timestamp1;
    else if (hour2 > hour1)
        return timestamp2;

    if (minute1 > minute2)
        return timestamp1;
    else if (minute2 > minute1)
        return timestamp2;

    if (second1 > second2)
        return timestamp1;
    else if (second2 > second1)
        return timestamp2;

    // If timestamps are equal, return the first one
    return timestamp1;
}

void add_file_to_dir(dir_info_bibak* dir_info, file_bibak file) {
    // Increase the total_file_count
    dir_info->total_file_count++;

    // Update the last_modified_time if the file's last_modified_time is the latest
    const char* latest_time = get_latest_timestamp(dir_info->last_modified_time, file.last_modified_time);
    strcpy(dir_info->last_modified_time, latest_time);

    // Reallocate memory for files array to accommodate the new file
    dir_info->files = realloc(dir_info->files, dir_info->total_file_count * sizeof(file_bibak));

    // Add the file to the files array
    file_bibak new_file;

    new_file.name = malloc(strlen(file.name) + 1);
    strcpy(new_file.name, file.name);

    strcpy(new_file.last_modified_time, file.last_modified_time);

    new_file.size = file.size;

    new_file.path = malloc(strlen(file.path) + 1);
    strcpy(new_file.path, file.path);

    dir_info->files[dir_info->total_file_count - 1] = new_file;
}

// Search the directory and generate a string representation of the directory information
int search_dir(const char* directory , dir_info_bibak* dir_info) {

    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("ERROR: Directory could not be opened!");
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, "log.txt")) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

            if (entry->d_type == DT_DIR) {
                // Recursively search inner directories
                search_dir(path, dir_info);
            } 
            
            else if (entry->d_type == DT_REG) {
                dir_info->total_file_count++;
                struct stat fileStat;

                char filePath[1024];
                sprintf(filePath, "%s/%s", directory, entry->d_name);

                if (stat(filePath, &fileStat) == 0) {
                    if (dir_info->files == NULL) {
                      
                        dir_info->files = malloc(sizeof(file_bibak));
                    } else {
                        file_bibak* new_files = malloc(dir_info->total_file_count * sizeof(file_bibak));
                        if (new_files == NULL) {
                            printf("errorrrrr\n");
                        }

                        // Copy the existing elements to the new block
                        for (int i = 0; i < dir_info->total_file_count - 1; i++) {
                            new_files[i] = dir_info->files[i];
                        }

                        // Free the memory occupied by the old block
                        free(dir_info->files);

                        // Update the files pointer to the new block
                        dir_info->files = new_files;
                    }
    
                    //Take file pointer
                    file_bibak* file = &(dir_info->files[dir_info->total_file_count - 1]);
                    
                    //Assign file name
                    file->name = malloc(strlen(entry->d_name) + 1);
                    strcpy(file->name, entry->d_name);

                    //Assign modified time

                    time_t modifiedTime = fileStat.st_mtime;
                    struct tm* timeInfo = localtime(&modifiedTime);
                    
                    strftime(file->last_modified_time, 20, "%Y-%m-%d %H:%M:%S", timeInfo);

                    //Assign file size
                    file->size = fileStat.st_size;

                    // Set file path
                    char relativePath[1024];
                    snprintf(relativePath, sizeof(relativePath), "%s", filePath);
                    file->path = malloc(strlen(relativePath) + 1);
                    strcpy(file->path, relativePath);
                    
                    //Modify the directory last modified date by controlling this file last modified date
                    const char* latest_modified = get_latest_timestamp(dir_info->last_modified_time,file->last_modified_time);
                    strcpy(dir_info->last_modified_time,latest_modified);

                }
            }
        }
    }

    closedir(dir);
    return 0;
}

// Convert dir_info to string
char* generate_dir_info_str(dir_info_bibak* dir_info) {
    // Allocate memory for the resulting string
    int buffer_size = 100000000;
    char* result = malloc(buffer_size);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }
    
    // Create the directory information string
    snprintf(result, buffer_size, "{\n");
    //buffer_reallocation(result,buffer_size);
    snprintf(result + strlen(result), buffer_size - strlen(result), "  total_file_count : %d,\n", dir_info->total_file_count);
    //buffer_reallocation(result,buffer_size);
    snprintf(result + strlen(result), buffer_size - strlen(result), "  last_modified_time : \"%s\",\n", dir_info->last_modified_time);
    //buffer_reallocation(result,buffer_size);
    snprintf(result + strlen(result), buffer_size - strlen(result), "  files : [\n");
    //buffer_reallocation(result,buffer_size);

    for (int i = 0; i < dir_info->total_file_count; i++) {
        snprintf(result + strlen(result), buffer_size - strlen(result), "    {\n");
        //buffer_reallocation(result,buffer_size);
        snprintf(result + strlen(result), buffer_size - strlen(result), "      name : \"%s\",\n", dir_info->files[i].name);
        //buffer_reallocation(result,buffer_size);
        snprintf(result + strlen(result), buffer_size - strlen(result), "      last_modified_time : \"%s\",\n", dir_info->files[i].last_modified_time);
        //buffer_reallocation(result,buffer_size);
        snprintf(result + strlen(result), buffer_size - strlen(result), "      size : %d,\n", dir_info->files[i].size);
        //buffer_reallocation(result,buffer_size);
        snprintf(result + strlen(result), buffer_size - strlen(result), "      path : \"%s\"\n", dir_info->files[i].path);
        //buffer_reallocation(result,buffer_size);
        snprintf(result + strlen(result), buffer_size - strlen(result), "    }%s\n", i == dir_info->total_file_count - 1 ? "" : ",");
        //buffer_reallocation(result,buffer_size);
    }

    snprintf(result + strlen(result), buffer_size - strlen(result), "  ]\n");
    //buffer_reallocation(result,buffer_size);
    snprintf(result + strlen(result), buffer_size - strlen(result), "}\n");
    //buffer_reallocation(result,buffer_size);

    return result;
}

// Convert string to dir_info
dir_info_bibak* parse_dir_info_str(const char* info_str) {
    dir_info_bibak* dir_info = malloc(sizeof(dir_info_bibak));
    dir_info->total_file_count = 0;
    dir_info->last_modified_time[0] = '\0';
    dir_info->files = NULL;

    // Parse the total file count
    const char* total_count_ptr = strstr(info_str, "total_file_count : ");
    if (total_count_ptr != NULL) {
        total_count_ptr += strlen("total_file_count : ");
        dir_info->total_file_count = atoi(total_count_ptr);
    }

    // Parse the last modified time
    const char* last_modified_ptr = strstr(info_str, "last_modified_time : \"");
    if (last_modified_ptr != NULL) {
        last_modified_ptr += strlen("last_modified_time : \"");
        const char* last_modified_end_ptr = strchr(last_modified_ptr, '\"');
        if (last_modified_end_ptr != NULL) {
            int last_modified_length = last_modified_end_ptr - last_modified_ptr;
            strncpy(dir_info->last_modified_time, last_modified_ptr, last_modified_length);
            dir_info->last_modified_time[last_modified_length] = '\0';
        }
    } else {
        strcpy(dir_info->last_modified_time, "");
    }

    // Parse the files
    const char* files_ptr = strstr(info_str, "files : [");
    if (files_ptr != NULL) {
        files_ptr += strlen("files : [");

        // Allocate memory for the files
        dir_info->files = malloc(dir_info->total_file_count * sizeof(file_bibak));

        if(dir_info->total_file_count != 0){
            for (int i = 0; i < dir_info->total_file_count; i++) {
                file_bibak* file = &(dir_info->files[i]);

                // Parse the file name
                const char* name_ptr = strstr(files_ptr, "name : \"");
                if (name_ptr != NULL) {
                    name_ptr += strlen("name : \"");
                    const char* name_end_ptr = strchr(name_ptr, '\"');
                    if (name_end_ptr != NULL) {
                        int name_length = name_end_ptr - name_ptr;                      
                        file->name = malloc((name_length + 1) * sizeof(char));
                        strncpy(file->name, name_ptr, name_length);
                        file->name[name_length] = '\0';
                    }
                }

                // Parse the last modified time
                const char* modified_time_ptr = strstr(files_ptr, "last_modified_time : \"");
                if (modified_time_ptr != NULL) {
                    modified_time_ptr += strlen("last_modified_time : \"");
                    strncpy(file->last_modified_time, modified_time_ptr, 19);
                    file->last_modified_time[19] = '\0';
                }

                // Parse the file size
                const char* size_ptr = strstr(files_ptr, "size : ");
                if (size_ptr != NULL) {
                    size_ptr += strlen("size : ");
                    file->size = atoi(size_ptr);
                }

                // Parse the file path
                const char* path_ptr = strstr(files_ptr, "path : \"");
                if (path_ptr != NULL) {
                    path_ptr += strlen("path : \"");
                    const char* path_end_ptr = strchr(path_ptr, '\"');
                    if (path_end_ptr != NULL) {
                        int path_length = path_end_ptr - path_ptr;
                        file->path = malloc((path_length + 1) * sizeof(char));
                        strncpy(file->path, path_ptr, path_length);
                        file->path[path_length] = '\0';
                    }
                }

                // Move to the next file
                files_ptr = strstr(files_ptr, "},") + strlen("},");
            }
        }
    }

    return dir_info;
}

//Read log file , parse it to dir_info and return dir_info
dir_info_bibak* read_log_file(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening log file");
        exit(1);
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file content
    char* file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Read the file content
    size_t read_size = fread(file_content, 1, file_size, file);
    file_content[read_size] = '\0';

    // Close the file
    fclose(file);
    
    // Parse the directory information from the file content
    dir_info_bibak* dir_info = parse_dir_info_str(file_content);

    // Free the allocated memory for file content
    free(file_content);

    return dir_info;
}

//Write the given dir_info by converting it to string
void write_log_file(const char* file_path, dir_info_bibak* dir_info) {
    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error opening log file");
        exit(1);
    }

    // Generate the string representation of the dir_info_bibak structure
    char* dir_info_str = generate_dir_info_str(dir_info);
    if (dir_info_str == NULL) {
        fclose(file);
        fprintf(stderr, "Failed to generate directory info string\n");
        exit(1);
    }

    // Write the string to the file
    fputs(dir_info_str, file);

    // Close the file
    fclose(file);

    // Free the allocated memory for the directory info string
    free(dir_info_str);
}

// Cleanup the allocated memory in dir_info
void cleanup_dir_info(dir_info_bibak* dir_info) {
    if (dir_info == NULL)
        return;

    if (dir_info->files != NULL) {
        for (int i = 0; i < dir_info->total_file_count; i++) {
            free(dir_info->files[i].name);
            free(dir_info->files[i].path);
        }
        free(dir_info->files);
    }

    free(dir_info);
}

/*
int main(){
    dir_info_bibak* dir_info = malloc(sizeof(dir_info_bibak));
    search_dir("./serverDir",dir_info);
    write_log_file("./serverDir/log.txt",dir_info);
    dir_info_bibak dir_info2 = read_log_file("./serverDir/log.txt");
    printf("%s",generate_dir_info_str(&dir_info2));
}*/