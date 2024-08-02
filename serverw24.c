#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h> // for PATH_MAX
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define MAX_COMMAND_LENGTH 100
#define MAX_RESPONSE_LENGTH 1024

int client_number = 1;

typedef struct {
    char name[PATH_MAX];
    time_t created_time;
} Directory;

// Comparator function for qsort
int compare_directories(const void *a, const void *b) {
    Directory *dirA = (Directory *)a;
    Directory *dirB = (Directory *)b;
    return difftime(dirA->created_time, dirB->created_time);
}

void process_client_request(int client_connection) {
    char client_command[MAX_COMMAND_LENGTH] = {0};
    char server_response[MAX_RESPONSE_LENGTH] = {0};

    while (1) {
        read(client_connection, client_command, MAX_COMMAND_LENGTH);

        // Clear response buffer before processing each command
        memset(server_response, 0, MAX_RESPONSE_LENGTH);

        // Check for command
        if (strcmp(client_command, "quitc\n") == 0) {
            printf("Client %d requested for quitting....\n", client_number);
            strcpy(server_response, "Quitting...\n");
            break;
        } else if (strncmp(client_command, "dirlist -a\n", 11) == 0) {
            DIR *dir;
            struct dirent *entry;
            if ((dir = opendir(".")) != NULL) {
                // Array to store directory names
                char directory_list[MAX_RESPONSE_LENGTH][PATH_MAX];
                int num_directories = 0;

                // Read directory names and store in the array
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        strcpy(directory_list[num_directories], entry->d_name);
                        num_directories++;
                    }
                }
                closedir(dir);

                // Sort directory names alphabetically
                qsort(directory_list, num_directories, sizeof(char[PATH_MAX]), strcmp);

                // Construct response with sorted directory names
                for (int i = 0; i < num_directories; i++) {
                    strcat(server_response, directory_list[i]);
                    strcat(server_response, "\n");
                }
            }
        } else if (strncmp(client_command, "dirlist -t\n", 11) == 0) {
            // List subdirectories in the order of creation time
            Directory directories[MAX_RESPONSE_LENGTH];
            int num_directories = 0;

            DIR *dir;
            struct dirent *entry;
            if ((dir = opendir(".")) != NULL) {
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        strcpy(directories[num_directories].name, entry->d_name);

                        // Get the creation time of the directory
                        struct stat st;
                        char path[PATH_MAX];
                        snprintf(path, PATH_MAX, "%s/%s", ".", entry->d_name);
                        stat(path, &st);
                        directories[num_directories].created_time = st.st_ctime;

                        num_directories++;
                    }
                }
                closedir(dir);
            }

            // Sort directories by creation time
            qsort(directories, num_directories, sizeof(Directory), compare_directories);

            // Construct response with sorted directory names
            for (int i = 0; i < num_directories; i++) {
                strcat(server_response, directories[i].name);
                strcat(server_response, "\n");
            }
        } else if (strncmp(client_command, "w24fn ", 6) == 0) {
            char filename[MAX_COMMAND_LENGTH];
            sscanf(client_command, "w24fn %s", filename);
            struct stat file_stat;
            if (stat(filename, &file_stat) == 0) {
                sprintf(server_response, "File: %s\nSize: %ld bytes\nCreated: %s\nPermissions: %o\n",
                        filename, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode);
            } else {
                strcpy(server_response, "Given Files  not found\n");
            }
        } else if (strncmp(client_command, "w24fz ", 6) == 0) {
            int size1, size2;
            sscanf(client_command, "w24fz %d %d", &size1, &size2);

            // Construct the find command to locate and compress files within the size range
            char find_command[MAX_COMMAND_LENGTH];
            sprintf(find_command, "find ~ -type f -size +%dc -size -%dc -exec tar -czf temp.tar.gz {} + 2>/dev/null", size1, size2);

            // Execute the find command
            FILE *fp = popen(find_command, "r");
            if (fp == NULL) {
                perror("Error occuring in  find command");
                strcpy(server_response, "Error executing find command\n");
            } else {
                sprintf(server_response, "Command to find and compress files between %d and %d bytes is running...\n", size1, size2);
                send(client_connection, server_response, strlen(server_response), 0);

                // Read and print ongoing command outputs
                while (fgets(server_response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    printf("Processing: %s", server_response);
                    send(client_connection, server_response, strlen(server_response), 0);
                }
                pclose(fp);
                memset(server_response, 0, MAX_RESPONSE_LENGTH);
            }
            send(client_connection, server_response, strlen(server_response), 0);
        } else if (strncmp(client_command, "w24ft ", 6) == 0) {
            char *extensions[4]; // Increased to accommodate extra slot for NULL terminator
            int count = 0;
            char *token = strtok(client_command + 6, " \n"); // Start parsing after "w24ft "

            // Parse up to 3 extensions from the command
            while (token != NULL && count < 3) {
                extensions[count++] = token;
                token = strtok(NULL, " \n");
            }
            extensions[count] = NULL; // Ensure the array is NULL-terminated

            if (count == 0) {
                strcpy(server_response, "please write extension \n");
            } else if (count == 4) {
                strcpy(server_response, "Exceeded maximum number of file extensions (3)\n");
            } else {
                // Create a command to find files with these extensions and archive them
                char find_command[MAX_COMMAND_LENGTH * 2];
                sprintf(find_command, "find ~ -type f \\( ");

                for (int i = 0; i < count; i++) {
                    sprintf(find_command + strlen(find_command), "-name \"*.%s\" ", extensions[i]);
                    if (i < count - 1) {
                        strcat(find_command, "-o ");
                    }
                }

                strcat(find_command, "\\) -exec tar -czf ft.tar.gz {} + 2>/dev/null");

                // Execute the command
                if (system(find_command) != 0) {
                    sprintf(server_response, "Failed to execute command to find and compress files.\n");
                } else {
                    struct stat st;
                    if (stat("ft.tar.gz", &st) == 0 && st.st_size > 0) {
                        sprintf(server_response, "Files compressed into ft.tar.gz\n");
                    } else {
                        sprintf(server_response, "No file found\n");
                    }
                }
            }
            send(client_connection, server_response, strlen(server_response), 0);
        } else if (strncmp(client_command, "w24fdb ", 6) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
    sscanf(client_command, "w24fdb %[^\n]", date_str);

    // Construct the find command to locate files with creation date <= given date
    // and compress them into temp.tar.gz
    char find_command[MAX_COMMAND_LENGTH * 2];
    snprintf(find_command, sizeof(find_command),
             "find ~ -type f -not -newerct '%s' -exec tar -czf db.tar.gz {} +", date_str);

    // Execute the find and tar command
    int command_result = system(find_command);

    // Check the result of the command
    if (command_result == 0) {
        // Send a success message to the client
        snprintf(server_response, sizeof(server_response), "db.tar.gz created successfully containing files created on or before %s.\n", date_str);
        send(client_connection, server_response, strlen(server_response), 0);

        // Open the created temp.tar.gz file
        FILE *tar_file = fopen("db.tar.gz", "rb");
        if (tar_file == NULL) {
            perror("Error opening db.tar.gz");
            strcpy(server_response, "Error opening db.tar.gz\n");
            send(client_connection, server_response, strlen(server_response), 0);
        } else {
            // Send the contents of temp.tar.gz to the client
            size_t bytes_read;
            while ((bytes_read = fread(server_response, 1, MAX_RESPONSE_LENGTH, tar_file)) > 0) {
                send(client_connection, server_response, bytes_read, 0);
            }

            // Close the tar file
            fclose(tar_file);

            // Delete the temp.tar.gz file to clean up
            remove("db.tar.gz");
        }
    } else {
        // Command execution failed
        strcpy(server_response, "Error creating temp.tar.gz for files created on or before the specified date.\n");
        send(client_connection, server_response, strlen(server_response), 0);
    }
 }  else if (strncmp(client_command, "w24fda ", 7) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
    sscanf(client_command, "w24fdb %[^\n]", date_str);

    // Construct the find command to locate files with creation date >= given date
    // and compress them into temp.tar.gz
    char find_command[MAX_COMMAND_LENGTH * 2];
    snprintf(find_command, sizeof(find_command),
             "find ~ -type f -newerct '%s' -or -newerct '%s 00:00' -exec tar -czf temp.tar.gz {} +", date_str, date_str);

    // Execute the find and tar command
    int command_result = system(find_command);

    // Check the result of the command
    if (command_result == 0) {
        // Send a success message to the client
        snprintf(server_response, sizeof(server_response), "temp.tar.gz created successfully containing files created on or after %s.\n", date_str);
        send(client_connection, server_response, strlen(server_response), 0);

        // Open the created temp.tar.gz file
        FILE *tar_file = fopen("temp.tar.gz", "rb");
        if (tar_file == NULL) {
            perror("Error opening temp.tar.gz");
            strcpy(server_response, "Error opening temp.tar.gz\n");
            send(client_connection, server_response, strlen(server_response), 0);
        } else {
            // Send the contents of temp.tar.gz to the client
            size_t bytes_read;
            while ((bytes_read = fread(server_response, 1, MAX_RESPONSE_LENGTH, tar_file)) > 0) {
                send(client_connection, server_response, bytes_read, 0);
            }

            // Close the tar file
            fclose(tar_file);

            // Delete the temp.tar.gz file to clean up
            remove("temp.tar.gz");
        }
    } else {
        // Command execution failed
        strcpy(server_response, "Error creating temp.tar.gz for files created on or after the specified date.\n");
        send(client_connection, server_response, strlen(server_response), 0);
    }
        } else {
            strcpy(server_response, "Command is not valid\n");
        }

        // Send response to client
        send(client_connection, server_response, strlen(server_response), 0);
        memset(server_response, 0, MAX_RESPONSE_LENGTH);
        memset(client_command, 0, MAX_COMMAND_LENGTH);
    }

    close(client_connection);
}

int main() {
    int server_connection, client_connection;
    struct sockaddr_in server_info;
    int server_info_length = sizeof(server_info);

    // Creating socket file descriptor
    if ((server_connection = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(PORT);

    // Binding socket to the specified port and server
    if (bind(server_connection, (struct sockaddr *)&server_info, sizeof(server_info)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening to incoming connections
    if (listen(server_connection, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for incoming connections...\n");

    while (1) {
        // Accepting incoming connection
        if ((client_connection = accept(server_connection, (struct sockaddr *)&server_info, (socklen_t*)&server_info_length)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client %d connected.\n", client_number);
        client_number++;

        // Forking a child process to handle client request
        if (fork() == 0) {
            close(server_connection);
            process_client_request(client_connection);
            printf("Client disconnected.\n");
            exit(0);
        } else {
            close(client_connection);
        }
    }

    return 0;
}
