#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_SERVERW24 8080
#define PORT_MIRROR1   8081
#define PORT_MIRROR2   8082
#define SERVERW24_IP   "127.0.0.1"
#define MIRROR1_IP     "127.0.0.1"
#define MIRROR2_IP     "127.0.0.1"
#define MAX_COMMAND_SIZE 100
#define MAX_RESPONSE_SIZE 1024
#define COUNT_FILE_PATH "connection_count.txt"

// Function to read the connection count from a file
int read_connection_count() {
    FILE *file = fopen(COUNT_FILE_PATH, "r");
    if (file == NULL) {
        // If file doesn't exist, create it and start from 0
        file = fopen(COUNT_FILE_PATH, "w");
        if (file == NULL) {
            perror("Error creating file");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%d", 0);
        fclose(file);
        return 0;
    }

    int count;
    fscanf(file, "%d", &count);
    fclose(file);
    return count;
}

// Function to write the connection count to a file
void write_connection_count(int count) {
    FILE *file = fopen(COUNT_FILE_PATH, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%d", count);
    fclose(file);
}

// Function to establish connection to a server
int connect_to_server(const char *server_ip, int server_port) {
    int client_socket;
    struct sockaddr_in server_addr;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        return -1;
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(client_socket);
        return -1;
    }

    return client_socket;
}

int main() {
    int connection_count = read_connection_count(); // Read connection count from file
    printf("Initial connection count: %d\n", connection_count);
     const char *server_ip;
        int server_port;
    int client_socket;
  if (connection_count < 3) {
            server_ip = SERVERW24_IP;
            server_port = PORT_SERVERW24;
             connection_count++;
       // printf("Updated connection count: %d\n", connection_count);
        write_connection_count(connection_count); 
        } else if (connection_count < 6) {
            server_ip = MIRROR1_IP;
            server_port = PORT_MIRROR1;
             connection_count++;
      //printf("Updated connection count: %d\n", connection_count);
        write_connection_count(connection_count); 
        } else if (connection_count < 9) {
            server_ip = MIRROR2_IP;
            server_port = PORT_MIRROR2;
             connection_count++;
      // printf("Updated connection count: %d\n", connection_count);
        write_connection_count(connection_count); 
        } else {
            int remaining = connection_count - 9;
            if (remaining % 3 == 0) {
                server_ip = SERVERW24_IP;
                server_port = PORT_SERVERW24;
            } else if (remaining % 3 == 1) {
                server_ip = MIRROR1_IP;
                server_port = PORT_MIRROR1;
            } else {
                server_ip = MIRROR2_IP;
                server_port = PORT_MIRROR2;
            }
             connection_count++;
      // printf("Updated connection count: %d\n", connection_count);
        write_connection_count(connection_count); 
        }

       
    while (1) {
        // Select server based on connection count
       

       // Connect to the selected server
        client_socket = connect_to_server(server_ip, server_port);
        if (client_socket < 0) {
            fprintf(stderr, "Failed to connect to server\n");
            exit(EXIT_FAILURE);
        }

        char command[MAX_COMMAND_SIZE];
        char response[MAX_RESPONSE_SIZE];

        // Handle communication with the server
        printf("Connected to server. Enter command: ");
        fgets(command, sizeof(command), stdin);
        send(client_socket, command, strlen(command), 0);

        // Receive and print server response
        int bytes_received = recv(client_socket, response, sizeof(response), 0);
        if (bytes_received < 0) {
            perror("recv failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            printf("Server disconnected\n");
            close(client_socket);
            exit(EXIT_FAILURE);
        } else {
            response[bytes_received] = '\0';
            printf("Server response:\n%s\n", response);
        }

        close(client_socket);
       // Write connection count to file
    }

    return 0;
}
