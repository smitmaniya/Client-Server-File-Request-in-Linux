
/*
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
#include <stdlib.h> // for qsort


#define PORT 8080
#define MAX_CLIENTS 5
#define MAX_COMMAND_LENGTH 100
#define MAX_RESPONSE_LENGTH 1024


int client_count = 1;

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
void crequest(int new_socket) {
    char buffer[MAX_COMMAND_LENGTH] = {0};
    char response[MAX_RESPONSE_LENGTH] = {0};

    while (1) {
        read(new_socket, buffer, MAX_COMMAND_LENGTH);

        // Clear response buffer before processing each command
        memset(response, 0, MAX_RESPONSE_LENGTH);

        // Check for command
        if (strcmp(buffer, "quitc\n") == 0) {
            printf("Client %d requested to quit.\n", client_count);
            strcpy(response, "Quitting...\n");
            break;
        } else if (strncmp(buffer, "dirlist -a\n", 11) == 0) {
            DIR *dir;
            struct dirent *entry;
            if ((dir = opendir(".")) != NULL) {
                // Array to store directory names
                char directories[MAX_RESPONSE_LENGTH][PATH_MAX];
                int num_directories = 0;

                // Read directory names and store in the array
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        strcpy(directories[num_directories], entry->d_name);
                        num_directories++;
                    }
                }
                closedir(dir);

                // Sort directory names alphabetically
                qsort(directories, num_directories, sizeof(char[PATH_MAX]), strcmp);

                // Construct response with sorted directory names
                for (int i = 0; i < num_directories; i++) {
                    strcat(response, directories[i]);
                    strcat(response, "\n");
                }
            }
        } else if (strncmp(buffer, "dirlist -t\n", 11) == 0) {
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
                strcat(response, directories[i].name);
                strcat(response, "\n");
            }
        }else if (strncmp(buffer, "w24fn ", 6) == 0) {
            char filename[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fn %s", filename);
            struct stat file_stat;
            if (stat(filename, &file_stat) == 0) {
                sprintf(response, "File: %s\nSize: %ld bytes\nCreated: %s\nPermissions: %o\n",
                        filename, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode);
            } else {
                strcpy(response, "File not found\n");
            }
        } else if (strncmp(buffer, "w24fz ", 6) == 0) {
    int size1, size2;
    sscanf(buffer, "w24fz %d %d", &size1, &size2);

    // Construct the command to find and compress files within the size range
    char cmd[MAX_COMMAND_LENGTH];
    sprintf(cmd, "find ~ -type f -size +%dc -size -%dc -exec tar -czf temp.tar.gz {} + 2>/dev/null", size1, size2); // Redirect stderr to /dev/null

    // Execute the command
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Error executing find command");
        strcpy(response, "Error executing find command\n");
    } else {
        sprintf(response, "Command to find and compress files between %d and %d bytes is running...\n", size1, size2);
        send(new_socket, response, strlen(response), 0);  // Send initial response

        // Read and print ongoing command outputs
        while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
            printf("Processing: %s", response);  // Print processing status
            send(new_socket, response, strlen(response), 0); // Send output to client
        }
        pclose(fp);
        sprintf(response, "Command completed.\n");
    }
    send(new_socket, response, strlen(response), 0); // Send completion message to client
}else if (strncmp(buffer, "w24ft ", 6) == 0) {
            char *extensions[3];
            int count = 0;
            char *token = strtok(buffer + 6, " \n"); // Start parsing after "w24ft "

            // Parse up to 3 extensions from the command
            while (token != NULL && count < 3) {
                extensions[count++] = token;
                token = strtok(NULL, " \n");
            }

            if (count == 0) {
                strcpy(response, "No file extension provided\n");
            } else {
                // Create a command to find files with these extensions and archive them
                char cmd[MAX_COMMAND_LENGTH * 2];
                sprintf(cmd, "find ~ -type f \\( ");

                for (int i = 0; i < count; i++) {
                    sprintf(cmd + strlen(cmd), "-name \"*.%s\" ", extensions[i]);
                    if (i < count - 1) {
                        strcat(cmd, "-o ");
                    }
                }

                strcat(cmd, "\\) -exec tar -czf ~/temp.tar.gz {} + 2>/dev/null");

                // Execute the command
                if (system(cmd) != 0) {
                    sprintf(response, "Failed to execute command to find and compress files.\n");
                } else {
                    struct stat st;
                    if (stat("~/temp.tar.gz", &st) == 0 && st.st_size > 0) {
                        sprintf(response, "Files compressed into temp.tar.gz\n");
                    } else {
                        sprintf(response, "No file found\n");
                    }
                }
            }
            send(new_socket, response, strlen(response), 0);
        } 
 else if (strncmp(buffer, "w24fdb ", 7) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fdb %[^\n]", date_str);
            struct tm tm;
            strptime(date_str, "%Y-%m-%d", &tm); // Include time.h for strptime function
            time_t date = mktime(&tm);
            
            char cmd[MAX_COMMAND_LENGTH * 2]; // Increase buffer size
            sprintf(cmd, "find ~ -type f -newermt '%s' -exec tar -czf temp.tar.gz {} +", date_str);
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Error executing find command");
                strcpy(response, "Error executing find command\n");
            } else {
                // Read command output
                while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    // Do nothing, just to clear the buffer
                }
                pclose(fp);
            }
        } else if (strncmp(buffer, "w24fda ", 7) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fda %[^\n]", date_str);
            struct tm tm;
            strptime(date_str, "%Y-%m-%d", &tm); // Include time.h for strptime function
            time_t date = mktime(&tm);
            
            char cmd[MAX_COMMAND_LENGTH * 2]; // Increase buffer size
            sprintf(cmd, "find ~ -type f ! -newermt '%s' -exec tar -czf temp.tar.gz {} +", date_str);
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Error executing find command");
                strcpy(response, "Error executing find command\n");
            } else {
                // Read command output
                while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    // Do nothing, just to clear the buffer
                }
                pclose(fp);
            }
        } else {
            strcpy(response, "Invalid command\n");
        }

        // Send response to client
        send(new_socket, response, strlen(response), 0);
    }

    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for incoming connections...\n");

    while (1) {
        // Accepting incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client %d connected.\n", client_count);
        client_count++;

        // Forking a child process to handle client request
        if (fork() == 0) {
            close(server_fd);
            crequest(new_socket);
            printf("Client disconnected.\n");
            exit(0);
        } else {
            close(new_socket);
        }
    }
	printf("Server waiting for incoming connections...\n");

   
    return 0;
}

*/

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

#define PORT 8081
#define MAX_CLIENTS 5
#define MAX_COMMAND_LENGTH 100
#define MAX_RESPONSE_LENGTH 1024

int client_count = 1;

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

void crequest(int new_socket) {
    char buffer[MAX_COMMAND_LENGTH] = {0};
    char response[MAX_RESPONSE_LENGTH] = {0};

    while (1) {
        read(new_socket, buffer, MAX_COMMAND_LENGTH);

        // Clear response buffer before processing each command
        memset(response, 0, MAX_RESPONSE_LENGTH);

        // Check for command
        if (strcmp(buffer, "quitc\n") == 0) {
            printf("Client %d requested to quit.\n", client_count);
            strcpy(response, "Quitting...\n");
            break;
        } else if (strncmp(buffer, "dirlist -a\n", 11) == 0) {
            DIR *dir;
            struct dirent *entry;
            if ((dir = opendir(".")) != NULL) {
                // Array to store directory names
                char directories[MAX_RESPONSE_LENGTH][PATH_MAX];
                int num_directories = 0;

                // Read directory names and store in the array
                while ((entry = readdir(dir)) != NULL) {
                    if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        strcpy(directories[num_directories], entry->d_name);
                        num_directories++;
                    }
                }
                closedir(dir);

                // Sort directory names alphabetically
                qsort(directories, num_directories, sizeof(char[PATH_MAX]), strcmp);

                // Construct response with sorted directory names
                for (int i = 0; i < num_directories; i++) {
                    strcat(response, directories[i]);
                    strcat(response, "\n");
                }
            }
        } else if (strncmp(buffer, "dirlist -t\n", 11) == 0) {
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
                strcat(response, directories[i].name);
                strcat(response, "\n");
            }
        } else if (strncmp(buffer, "w24fn ", 6) == 0) {
            char filename[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fn %s", filename);
            struct stat file_stat;
            if (stat(filename, &file_stat) == 0) {
                sprintf(response, "File: %s\nSize: %ld bytes\nCreated: %s\nPermissions: %o\n",
                        filename, file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode);
            } else {
                strcpy(response, "File not found\n");
            }
        } else if (strncmp(buffer, "w24fz ", 6) == 0) {
            int size1, size2;
            sscanf(buffer, "w24fz %d %d", &size1, &size2);

            // Construct the command to find and compress files within the size range
            char cmd[MAX_COMMAND_LENGTH];
            sprintf(cmd, "find ~ -type f -size +%dc -size -%dc -exec tar -czf temp.tar.gz {} + 2>/dev/null", size1, size2); // Redirect stderr to /dev/null

            // Execute the command
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Error executing find command");
                strcpy(response, "Error executing find command\n");
            } else {
                sprintf(response, "Command to find and compress files between %d and %d bytes is running...\n", size1, size2);
                send(new_socket, response, strlen(response), 0);  // Send initial response

                // Read and print ongoing command outputs
                while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    printf("Processing: %s", response);  // Print processing status
                    send(new_socket, response, strlen(response), 0); // Send output to client
                }
                pclose(fp);
                 memset(response, 0, MAX_RESPONSE_LENGTH);
               // strcpy(response, "Command completed.\n"); // Reset response message
            }
            send(new_socket, response, strlen(response), 0);
            //buffer[MAX_COMMAND_LENGTH] = {0}; // Send completion message to client
        } else if (strncmp(buffer, "w24ft ", 6) == 0) {
            char *extensions[4]; // Increased to accommodate extra slot for NULL terminator
            int count = 0;
            char *token = strtok(buffer + 6, " \n"); // Start parsing after "w24ft "

            // Parse up to 3 extensions from the command
            while (token != NULL && count < 3) {
                extensions[count++] = token;
                token = strtok(NULL, " \n");
            }
            extensions[count] = NULL; // Ensure the array is NULL-terminated

            if (count == 0) {
                strcpy(response, "No file extension provided\n");
            } else if (count == 4) {
                strcpy(response, "Exceeded maximum number of file extensions (3)\n");
            } else {
                // Create a command to find files with these extensions and archive them
                char cmd[MAX_COMMAND_LENGTH * 2];
                sprintf(cmd, "find ~ -type f \\( ");

                for (int i = 0; i < count; i++) {
                    sprintf(cmd + strlen(cmd), "-name \"*.%s\" ", extensions[i]);
                    if (i < count - 1) {
                        strcat(cmd, "-o ");
                    }
                }

                strcat(cmd, "\\) -exec tar -czf ft.tar.gz {} + 2>/dev/null");

                // Execute the command
                if (system(cmd) != 0) {
                    sprintf(response, "Failed to execute command to find and compress files.\n");
                } else {
                    struct stat st;
                    if (stat("temp.tar.gz", &st) == 0 && st.st_size > 0) {
                        sprintf(response, "Files compressed into temp.tar.gz\n");
                    } else {
                        sprintf(response, "No file found\n");
                    }
                }
            }
            send(new_socket, response, strlen(response), 0);
            //buffer[MAX_COMMAND_LENGTH] = {0};
        } else if (strncmp(buffer, "w24fdb ", 7) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fdb %[^\n]", date_str);
            struct tm tm;
            strptime(date_str, "%Y-%m-%d", &tm); // Include time.h for strptime function
            time_t date = mktime(&tm);

            char cmd[MAX_COMMAND_LENGTH * 2]; // Increase buffer size
            sprintf(cmd, "find ~ -type f -newermt '%s' -exec tar -czf temp.tar.gz {} +", date_str);
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Error executing find command");
                strcpy(response, "Error executing find command\n");
            } else {
                // Read command output
                while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    // Do nothing, just to clear the buffer
                }
                pclose(fp);
            }
        } else if (strncmp(buffer, "w24fda ", 7) == 0) {
            char date_str[MAX_COMMAND_LENGTH];
            sscanf(buffer, "w24fda %[^\n]", date_str);
            struct tm tm;
            strptime(date_str, "%Y-%m-%d", &tm); // Include time.h for strptime function
            time_t date = mktime(&tm);

            char cmd[MAX_COMMAND_LENGTH * 2]; // Increase buffer size
            sprintf(cmd, "find ~ -type f ! -newermt '%s' -exec tar -czf temp.tar.gz {} +", date_str);
            FILE *fp = popen(cmd, "r");
            if (fp == NULL) {
                perror("Error executing find command");
                strcpy(response, "Error executing find command\n");
            } else {
                // Read command output
                while (fgets(response, MAX_RESPONSE_LENGTH, fp) != NULL) {
                    // Do nothing, just to clear the buffer
                }
                pclose(fp);
            }
        } else {
            strcpy(response, "Invalid command\n");
        }

        // Send response to client
        send(new_socket, response, strlen(response), 0);
        memset(response, 0, MAX_RESPONSE_LENGTH);
         memset(buffer, 0, MAX_COMMAND_LENGTH);
    }

    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for incoming connections...\n");

    while (1) {
        // Accepting incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client %d connected.\n", client_count);
        client_count++;

        // Forking a child process to handle client request
        if (fork() == 0) {
            close(server_fd);
            crequest(new_socket);
            printf("Client disconnected.\n");
            exit(0);
        } else {
            close(new_socket);
        }
    }

    return 0;
}

