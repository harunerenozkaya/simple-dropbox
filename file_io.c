#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <utime.h>

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

// Open the file which will be downloaded from server and create if the inner directories needed
FILE* create_file_and_dir(file_bibak file , char* dir_name) {
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

// Extract base and relative part of the path
char* extract_local_dir_path_part_str(char* path,const char* local_dir) {
    // Calculate the length of the local directory
    int local_dir_len = strlen(local_dir);

    // Calculate the length of the remaining path
    int remaining_path_len = strlen(path) - local_dir_len;

    // Allocate memory for the relative path
    char* relative_path = (char*)malloc(remaining_path_len + 1);

    // Copy the remaining path to the relative path variable
    strcpy(relative_path, path + local_dir_len);

    return relative_path;
}

// Conconacete base and relative part of the path
void append_local_dir_path_part_str(char* path, const char* local_dir) {
    // Calculate the length of the local directory
    int local_dir_len = strlen(local_dir);

    // Calculate the length of the relative path
    int relative_path_len = strlen(path);

    // Calculate the total length of the updated file path
    int updated_file_path_len = local_dir_len + relative_path_len;

    // Allocate memory for the updated file path
    char* updated_file_path = (char*)malloc(updated_file_path_len + 1);

    // Copy the local directory to the updated file path
    strcpy(updated_file_path, local_dir);

    // Concatenate the relative path to the updated file path
    strcat(updated_file_path, path);

    // Update the file path in the request structure
    strcpy(path, updated_file_path);

    // Free the memory allocated for the updated file path
    free(updated_file_path);
}

// Parse the date time str to tm_struct
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

// Change the last modification time of the given file
int change_last_modification_time(char* file_path, const char* directory ,const char *last_modification_time) {

    // Concatenate the directory and file path
    int total_length = strlen(directory) + strlen(file_path) + 1; // +1 for '/', +1 for '\0'
    char* real_path = (char*)malloc(total_length * sizeof(char));
    snprintf(real_path, total_length, "%s%s", directory, file_path);

    // Parse the given time
    struct tm mod_time_struct;
    if (parse_date_time(last_modification_time, &mod_time_struct) == -1) {
        return -1;
    }

    // Assign the time
    struct utimbuf new_times;
    new_times.actime = 0; // Access time (set to 0 to keep it unchanged)
    new_times.modtime = mktime(&mod_time_struct); // New modified time (in seconds since the epoch)
    
    // Change the time
    if (utime(real_path, &new_times) != 0) {
        perror("utime");
        free(real_path);
        return 1;
    }

    free(real_path);
    return 0;
}

// Read the file and return the content
char* read_file_data(const char* path) {

    // Open the file in binary mode for reading
    FILE* file = fopen(path, "rb"); 
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END); 
    long file_size = ftell(file); 
    fseek(file, 0, SEEK_SET); 

    // Allocate memory for the file data
    char* file_data = (char*)malloc(file_size + 1); 
    if (file_data == NULL) {
        fclose(file);
        perror("Error allocating memory for file data");
        return NULL;
    }

    // Read the file data into the allocated memory
    size_t read_size = fread(file_data, 1, file_size, file); 
    if (read_size != file_size) {
        fclose(file);
        free(file_data);
        perror("Error reading file");
        return NULL;
    }

    file_data[file_size] = '\0';

    fclose(file);
    return file_data;
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

// Add the gien file to dir_info
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

    // Assign values of the file
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
char* generate_dir_info_str(dir_info_bibak* dir_info , int isRelative ,const char* local_dir) {
    // Allocate memory for the resulting string
    int buffer_size = 100000000;
    char* result = malloc(buffer_size);
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return NULL;
    }
    
    // Create the directory information string
    snprintf(result, buffer_size, "{\n");
    snprintf(result + strlen(result), buffer_size - strlen(result), "  total_file_count : %d,\n", dir_info->total_file_count);
    snprintf(result + strlen(result), buffer_size - strlen(result), "  last_modified_time : \"%s\",\n", dir_info->last_modified_time);
    snprintf(result + strlen(result), buffer_size - strlen(result), "  files : [\n");

    for (int i = 0; i < dir_info->total_file_count; i++) {
        snprintf(result + strlen(result), buffer_size - strlen(result), "    {\n");

        snprintf(result + strlen(result), buffer_size - strlen(result), "      name : \"%s\",\n", dir_info->files[i].name);

        snprintf(result + strlen(result), buffer_size - strlen(result), "      last_modified_time : \"%s\",\n", dir_info->files[i].last_modified_time);

        snprintf(result + strlen(result), buffer_size - strlen(result), "      size : %d,\n", dir_info->files[i].size);

        if(isRelative == 0){
            snprintf(result + strlen(result), buffer_size - strlen(result), "      path : \"%s\"\n", dir_info->files[i].path);
        }
        else{
            char* temp = malloc(strlen(dir_info->files[i].path) + 1);
            strcpy(temp,dir_info->files[i].path);
            free(dir_info->files[i].path);

            dir_info->files[i].path = extract_local_dir_path_part_str(temp,local_dir);

            free(temp);

            snprintf(result + strlen(result), buffer_size - strlen(result), "      path : \"%s\"\n", dir_info->files[i].path);
        }

        snprintf(result + strlen(result), buffer_size - strlen(result), "    }%s\n", i == dir_info->total_file_count - 1 ? "" : ",");
    }

    snprintf(result + strlen(result), buffer_size - strlen(result), "  ]\n");
    snprintf(result + strlen(result), buffer_size - strlen(result), "}\n");

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

//Write the given dir_info to log file by converting it to string
void write_log_file(const char* file_path, dir_info_bibak* dir_info) {
    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error opening log file");
        exit(1);
    }

    // Generate the string representation of the dir_info_bibak structure
    char* dir_info_str = generate_dir_info_str(dir_info ,0,"");
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

