#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    char* name;
    char last_modified_time[20];
    int size;
    char* path; // Added path field
} file_bibak;

typedef struct {
    int total_file_count;
    char last_modified_time[20];
    file_bibak* files;
} dir_info_bibak;


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

// Search the directory and generate a string representation of the directory information
int search_dir(const char* directory , dir_info_bibak* dir_info) {

    DIR* dir = opendir(directory);
    if (dir == NULL) {
        perror("ERROR: Directory could not be opened!");
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

            if (entry->d_type == DT_DIR) {
                // Recursively search inner directories
                search_dir(path, dir_info);
            } 
            
            else if (entry->d_type == DT_REG) {
                dir_info->total_file_count++;
                struct stat fileStat;

                char filePath[256];
                sprintf(filePath, "%s/%s", directory, entry->d_name);

                if (stat(filePath, &fileStat) == 0) {
                    if (dir_info->files == NULL) {
                        dir_info->files = malloc(sizeof(file_bibak));
                    } else {
                        dir_info->files = realloc(dir_info->files, dir_info->total_file_count * sizeof(file_bibak));
                    }

                    //Take file pointer
                    file_bibak* file = &(dir_info->files[dir_info->total_file_count - 1]);
                    
                    //Assign file name
                    file->name = strdup(entry->d_name);

                    //Assign modified time

                    time_t modifiedTime = fileStat.st_mtime;
                    struct tm* timeInfo = localtime(&modifiedTime);

                    strftime(file->last_modified_time, 20, "%Y-%m-%d %H:%M:%S", timeInfo);

                    //Assign file size
                    file->size = fileStat.st_size;

                    // Set file path
                    char relativePath[1024];
                    snprintf(relativePath, sizeof(relativePath), "%s", filePath);
                    file->path = strdup(relativePath);
                    
                    //Modify the directory last modified date by controlling this file last modified date
                    const char* latest_modified = get_latest_timestamp(dir_info->last_modified_time,file->last_modified_time);
                    snprintf(dir_info->last_modified_time,20,"%s",latest_modified);

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
    int buffer_size = 1024;
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
        snprintf(result + strlen(result), buffer_size - strlen(result), "      path : \"%s\"\n", dir_info->files[i].path);
        snprintf(result + strlen(result), buffer_size - strlen(result), "    }%s\n", i == dir_info->total_file_count - 1 ? "" : ",");
    }

    snprintf(result + strlen(result), buffer_size - strlen(result), "  ]\n");
    snprintf(result + strlen(result), buffer_size - strlen(result), "}\n");

    printf("%s",result);

    return result;
}

int main(){
    dir_info_bibak* dir_info = malloc(sizeof(dir_info_bibak));
    search_dir("./serverDir",dir_info);
    generate_dir_info_str(dir_info);
}