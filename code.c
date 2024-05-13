#include <stdio.h>     
#include <string.h>     
#include <fcntl.h>     
#include <unistd.h>     
#include <sys/stat.h>  
#include <sys/types.h> 
#include <dirent.h> 
#include <stdlib.h>    
#include <time.h>    

#define MAX_DIRECTORIES 10

void move_to_quarantine(char* file_path);
void print_indent(int depth, FILE *file) 
{
    for(int i=0; i<depth; ++i) 
    {
        fprintf(file, "│   ");
    }
}

void parse_folder(char* folder_path, int depth, FILE *file) 
{
    DIR* parent=opendir(folder_path);
    if(parent==NULL) 
    {
        perror("Error opening directory");
        return;
    }
    struct dirent* entry;
    char path[1024];
    while((entry=readdir(parent))!=NULL) 
    {
        if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0) 
        {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", folder_path, entry->d_name);
        struct stat statbuf;
        if(stat(path, &statbuf)==-1) 
        {
            perror("Error getting file status");
            continue;
        }
        if(S_ISDIR(statbuf.st_mode)) 
        {
            print_indent(depth, file);
            fprintf(file, "├── %s (Directory)\n", entry->d_name);
            parse_folder(path, depth + 1, file);
        } 
        else 
        {
            print_indent(depth, file);
            fprintf(file, "├── %s (%ld bytes)\n", entry->d_name, (long)statbuf.st_size);
            fprintf(file, "│   Last Modified: %s", ctime(&statbuf.st_mtime));
            fprintf(file, "│   Permissions: ");
            fprintf(file, (S_ISDIR(statbuf.st_mode)) ? "d" : "-");
            fprintf(file, (statbuf.st_mode & S_IRUSR) ? "r" : "-");
            fprintf(file, (statbuf.st_mode & S_IWUSR) ? "w" : "-");
            fprintf(file, (statbuf.st_mode & S_IXUSR) ? "x" : "-");
            fprintf(file, (statbuf.st_mode & S_IRGRP) ? "r" : "-");
            fprintf(file, (statbuf.st_mode & S_IWGRP) ? "w" : "-");
            fprintf(file, (statbuf.st_mode & S_IXGRP) ? "x" : "-");
            fprintf(file, (statbuf.st_mode & S_IROTH) ? "r" : "-");
            fprintf(file, (statbuf.st_mode & S_IWOTH) ? "w" : "-");
            fprintf(file, (statbuf.st_mode & S_IXOTH) ? "x" : "-");
            fprintf(file, "\n");
            fprintf(file, "│   Inode no: %ld\n", (long)statbuf.st_ino);
            char command[200];
            sprintf(command, "./script.sh \"%s\"", path);
            FILE *permission_check=popen(command, "r");
            if(permission_check==NULL) 
            {
                perror("Failed to execute permission check script");
                continue;
            }
            char output[10];
            fgets(output, sizeof(output), permission_check);
            pclose(permission_check);
            int errorCode=atoi(output);
            switch (errorCode) 
            {
                case 0:
                    fprintf(file, "│   No error\n");
                    break;
                case 1:
                    fprintf(file, "│   Error: File does not exist\n");
                    break;
                case 2:
                    fprintf(file, "│   Error: File is not readable\n");
                    break;
                case 3:
                    fprintf(file, "│   Error: File is not writable\n");
                    break;
                case 4:
                    fprintf(file, "│   Error: File contains suspicious words\n");
                    move_to_quarantine(path);
                    break;
                default:
                    fprintf(file, "│   Unknown error\n");
                    break;
            }
        }
    }
    closedir(parent);
}

void move_to_quarantine(char* file_path) 
{
    const char* quarantine_folder = "quarantine";
    struct stat st={0};
    if(stat(quarantine_folder, &st)==-1) 
    {
        if(mkdir(quarantine_folder)==-1) 
        {
            perror("Error creating quarantine folder");
            return;
        }
    }
    char* file_name=strrchr(file_path, '/');
    if(file_name==NULL) 
    {
        file_name=file_path;
    } 
    else 
    {
        file_name++;
    }
    char destination_path[1024];
    snprintf(destination_path, sizeof(destination_path), "%s/%s", quarantine_folder, file_name);
    if(rename(file_path, destination_path)!=0) 
    {
        perror("Error moving file to quarantine folder");
    }
}
void update_snapshot(char *dir_path, char *output_directory) 
{
    char snapshot_file_path[1024];
    time_t current_time;
    struct tm *time_info;
    time(&current_time);
    time_info = localtime(&current_time);
    snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/Snapshot_%s.txt", output_directory, dir_path);
    FILE *snapshot_file = fopen(snapshot_file_path, "w");
    if (snapshot_file==NULL) 
    {
        perror("Error opening snapshot file for writing");
        return;
    }
    fprintf(snapshot_file, "Timestamp: %s", asctime(time_info));
    fprintf(snapshot_file, "Entry: %s\n", dir_path);
    fprintf(snapshot_file, "\nDirectory Contents:\n");
    parse_folder(dir_path, 1, snapshot_file);
    fclose(snapshot_file);
}

int main(int argc, char** argv)
{
    if(argc<4 || argc>12) 
    {
        fprintf(stderr, "Wrong format amigo\n");
        return 1;
    }
    char* output_directory=NULL;
    char* directories[MAX_DIRECTORIES];
    int num_directories=0;
    for(int i=1; i<argc; i++) 
    {
        if(strcmp(argv[i], "-o")==0) 
        {
            i++;
            if(i>=argc) 
            {
                fprintf(stderr, "Error: Put a name to that directory you want the snapshots to be in XD\n");
                return 1;
            }
            output_directory=argv[i];
        }
         else 
        {
            if(num_directories>=MAX_DIRECTORIES) 
            {
                fprintf(stderr, "Error: You wrote a bit too many directories\n");
                return 1;
            }
            directories[num_directories++]=argv[i];
        }
    }

    if(output_directory==NULL)
    {
        fprintf(stderr, "Error: Output directory not specified\n");
        return 1;
    }
    struct stat st;
    if(stat(output_directory, &st)==0 && S_ISDIR(st.st_mode)) 
    {
        printf("Output directory '%s' already exists. Snapshots will be stored anyway.\n", output_directory);
    }
     else 
    {
        if(mkdir(output_directory)==-1) 
        {
            perror("Error creating output directory");
            return 1;
        }
    }

    for(int i=0; i<num_directories; i++) 
    {
        update_snapshot(directories[i], output_directory);
    }
    return 0;
}
