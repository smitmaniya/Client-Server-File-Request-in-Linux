#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT_SERVERW24 8080
#define PORT_MIRROR1   8081
#define PORT_MIRROR2   8082
#define SERVERW24_IP   "127.0.0.1"
#define MIRROR1_IP     "127.0.0.1"
#define MIRROR2_IP     "127.0.0.1"
#define MAX_COMMAND_LENGTH 100
#define MAX_RESPONSE_LENGTH 1024
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

void send_command(int sock, char *command) {
    send(sock, command, strlen(command), 0);
}

void receive_response(int sock, char *buffer) {
    read(sock, buffer, MAX_RESPONSE_LENGTH);
}

int main() {
    int connection_count = read_connection_count(); // Read connection count from file
    printf("Initial connection count: %d\n", connection_count);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char command[MAX_COMMAND_LENGTH];
    char response[MAX_RESPONSE_LENGTH] = {0};

    switch (connection_count) {
        case 0:
        case 1:
        case 2:
           // printf("ss");
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_SERVERW24);
            inet_pton(AF_INET, SERVERW24_IP, &serv_addr.sin_addr);
            connection_count++;
            write_connection_count(connection_count);
            break;
        case 3:
        case 4:
        case 5:
           // printf("sd");
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_MIRROR1);
            inet_pton(AF_INET, MIRROR1_IP, &serv_addr.sin_addr);
            connection_count++;
            write_connection_count(connection_count);
            break;
        case 6:
        case 7:
        case 8:
            //printf("sa");
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_MIRROR2);
            inet_pton(AF_INET,P, &serv_addr.sin_addr);
            connection_count++;
            write_connection_count(connection_count);
            break;
        default:
        int remaining = connection_count - 9;
            if (remaining % 3 == 0) {
               serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_SERVERW24);
            inet_pton(AF_INET, SERVERW24_IP, &serv_addr.sin_addr);
            } else if (remaining % 3 == 1) {
               serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_MIRROR1);
            inet_pton(AF_INET, MIRROR1_IP, &serv_addr.sin_addr);
            } else {
                serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT_MIRROR2);
            inet_pton(AF_INET, MIRROR2_IP, &serv_addr.sin_addr);
            }

            connection_count++;
            write_connection_count(connection_count);
           break;
    }

    // Creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Connecting to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server.\n");

    while (1) {
        printf("Enter command: ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);

        // Sending command to server
        send_command(sock, command);

        // Receiving response from server
        memset(response, 0, MAX_RESPONSE_LENGTH); // Clear response buffer
        receive_response(sock, response);

        printf("Server response: %s\n", response);

        // Quit if command is "quitc"
        if (strcmp(command, "quitc\n") == 0) {
            printf("Disconnecting from server.\n");
            break;
        }
    }

    close(sock);
    return 0;
}

