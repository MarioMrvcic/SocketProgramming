#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#define MAX_UDP_THREAD_COUNT 25
#define MAX_TCP_THREAD_COUNT 25
#define BACKLOG 5
#define MAX_MESSAGE_SIZE 1024
#define MAX_COMMAND_SIZE 50

char payload[MAX_MESSAGE_SIZE] = "payload";
pthread_mutex_t mutex;

struct data_to_handle_UDP {
    char *message;
    ssize_t message_size;
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len;
    int UDP_socket;
};

struct data_to_handle_TCP {
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len;
    int TCP_socket, incoming_socket;
};

// Function that handles UDP messages
void *handle_UDP_message(void *arg) {

    // Cast the argument to a pointer to the data_to_handle_UDP structure
    struct data_to_handle_UDP *handle_UDP_data = (struct data_to_handle_UDP *)arg;

    printf("Recieved message: ");
    for(int i=0; i<handle_UDP_data->message_size; ++i){
        printf("%c", handle_UDP_data->message[i]);
    }
    printf("\n");

    if(strcmp(handle_UDP_data->message, "HELLO\n") == 0){
        char current_payload[MAX_MESSAGE_SIZE];
        // Locking the mutex for synchronisation issues and checking the current payload
        pthread_mutex_lock(&mutex);
        strcpy(current_payload, payload);
        pthread_mutex_unlock(&mutex);

        // Sending payload to bot
        if (sendto(handle_UDP_data->UDP_socket, current_payload, strlen(current_payload), 0, (const struct sockaddr *)&handle_UDP_data->incoming_addr, sizeof(handle_UDP_data->incoming_addr)) < 0) {
            perror("Send message failed");
        } else {
            // Usually use inet_ntop() instead of inet_ntoa(), but for simplicity we use inet_ntoa()
            printf("\"%s\" message sent to %s:%d\n", current_payload, inet_ntoa(handle_UDP_data->incoming_addr.sin_addr), ntohs(handle_UDP_data->incoming_addr.sin_port));
        }
    }
    else{
        printf("Recieved message: ");
        for(int i=0; i<handle_UDP_data->message_size; ++i){
            printf("%c", handle_UDP_data->message[i]);
        }
        printf("\n");
    }

    // Free the message memory
    free(handle_UDP_data->message);
    // Free the allocated data structure memory
    free(handle_UDP_data);

    pthread_exit(NULL);
}

// Listening for UDP messages and handling with threads
void *listen_for_UDP_message(void *arg){    

    // Dereference the argument to get the socket descriptor
    int UDP_socket = *(int *)arg;

    // Keeping track of active threads
    pthread_t UDP_threads[MAX_UDP_THREAD_COUNT];
    int UDP_thread_id = 0;

    // Initialize the struct where the incoming address will be stored
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len = sizeof(incoming_addr);

    // Initialize message buffer and continuously listen for UDP messages
    char UDP_message_buffer[MAX_MESSAGE_SIZE];
    while(true){
        // Receive UDP message
        ssize_t UDP_message_size = recvfrom(UDP_socket, UDP_message_buffer, sizeof(UDP_message_buffer) - 1, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len);
        if (UDP_message_size < 0) {
            // If nothing is received, we continue
            continue;
        } else {
            // Null terminate the string
            UDP_message_buffer[UDP_message_size] = '\0';

            // If max number of threads is exceeded, start from the beginning
            if(UDP_thread_id >= MAX_UDP_THREAD_COUNT){
                UDP_thread_id = 0;
            }

            // Allocate memory for struct because of potential race Condition and initialize all variables that will be passed to the new thread
            struct data_to_handle_UDP *handle_UDP_data = malloc(sizeof(struct data_to_handle_UDP));
            if (handle_UDP_data == NULL) {
                perror("Failed to allocate memory");
                continue;
            }
            // Allocate memory for the message based on the size received
            handle_UDP_data->message = malloc(UDP_message_size);
            if (handle_UDP_data->message == NULL) {
                perror("Failed to allocate memory for message");
                free(handle_UDP_data);
                continue;
            }
            memcpy(handle_UDP_data->message, UDP_message_buffer, UDP_message_size);
            handle_UDP_data->message_size = UDP_message_size;
            handle_UDP_data->incoming_addr = incoming_addr;
            handle_UDP_data->incoming_addr_len = incoming_addr_len;
            handle_UDP_data->UDP_socket = UDP_socket;

            // Create a thread to handle the UDP message
            if (pthread_create(&UDP_threads[UDP_thread_id], NULL, handle_UDP_message, (void *)handle_UDP_data) != 0) {
                perror("Failed to create thread");
                free(handle_UDP_data->message);
                free(handle_UDP_data);
                continue;
            }

            // Increase thread id for the next iteration
            UDP_thread_id++;
        }
    }
}

// Function that handles TCP connection
void *handle_TCP_connection(void *arg) {

    // Cast the argument to a pointer to the data_to_handle_UDP structure
    struct data_to_handle_TCP *handle_TCP_data = (struct data_to_handle_TCP *)arg;

    printf("TCP connection established with %s\n", inet_ntoa(handle_TCP_data->incoming_addr.sin_addr));

    char recieved_TCP_message_buffer[MAX_MESSAGE_SIZE] = {0};
    int bytes_read;

    // Read data from the client
    if ((bytes_read = read(handle_TCP_data->incoming_socket, recieved_TCP_message_buffer, sizeof(recieved_TCP_message_buffer))) > 0) {
        // Remove after checking
        printf("Received TCP message: %s\n", recieved_TCP_message_buffer);
        if(strcmp(recieved_TCP_message_buffer, "HELLO\n") == 0){
            char current_payload[MAX_MESSAGE_SIZE];
            // Locking the mutex for synchronisation issues and checking the current payload
            pthread_mutex_lock(&mutex);
            strcpy(current_payload, payload);
            pthread_mutex_unlock(&mutex);
            send(handle_TCP_data->incoming_socket, current_payload, sizeof(current_payload), 0);
        }
        else{
            printf("Received TCP message: %s\n", recieved_TCP_message_buffer);
        }
    }else{
        perror("Error while reading TCP message\n");
    }

    // Close sockets
    close(handle_TCP_data->incoming_socket);
    // Free the allocated data structure memory
    free(handle_TCP_data);

    pthread_exit(NULL);
}

// Listening for TCP connections
void *listen_for_TCP_connections(void *arg){    

    // Dereference the argument to get the socket descriptor
    int TCP_socket = *(int *)arg;

    // Keeping track of active threads
    pthread_t TCP_threads[MAX_TCP_THREAD_COUNT];
    int TCP_thread_id = 0;

    // Initialize the struct and socket where the incoming address will be stored
    int incoming_socket;
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len = sizeof(incoming_addr);

    // Listening for incoming connections
    if (listen(TCP_socket, BACKLOG) < 0) {
        perror("Listen failed");
        close(TCP_socket);
    }

    while(true){

        // Accept a new connection
        if ((incoming_socket = accept(TCP_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for struct because of potential race Condition and initialize all variables that will be passed to the new thread
        struct data_to_handle_TCP *handle_TCP_data = malloc(sizeof(struct data_to_handle_TCP));
        if (handle_TCP_data == NULL) {
            perror("Failed to allocate memory");
            continue;
        }
        handle_TCP_data->incoming_addr = incoming_addr;
        handle_TCP_data->incoming_addr_len = incoming_addr_len;
        handle_TCP_data->incoming_socket = incoming_socket;
        handle_TCP_data->TCP_socket = TCP_socket;
        // Create a thread to handle the UDP message
        if (pthread_create(&TCP_threads[TCP_thread_id], NULL, handle_TCP_connection, (void *)handle_TCP_data) != 0) {
            perror("Failed to create thread");
            free(handle_TCP_data);
            continue;
        }
        // Increase thread id for the next iteration
        TCP_thread_id++;   
    }
}

// Function to handle commands from stdin
void handle_stdin_command(char *stdin_command){

    // Remove trailing newline character
    stdin_command[strcspn(stdin_command, "\n")] = '\0';

    // Handle commands
    if (strncmp(stdin_command, "PRINT", 5) == 0) {
        char current_payload[MAX_MESSAGE_SIZE];
        // Locking the mutex for synchronisation issues and checking the current payload
        pthread_mutex_lock(&mutex);
        strcpy(current_payload, payload);
        pthread_mutex_unlock(&mutex);
        printf("Payload is: \"%s\"\n", current_payload);

    } else if (strncmp(stdin_command, "SET ", 4) == 0) {
        char current_payload[MAX_MESSAGE_SIZE];
        // Locking the mutex for synchronisation issues and checking the current payload
        pthread_mutex_lock(&mutex);
        strcpy(payload, strdup(stdin_command + 4));
        strcpy(current_payload, payload);
        pthread_mutex_unlock(&mutex);
        printf("Payload set to: \"%s\"\n", current_payload);

    } else if (strncmp(stdin_command, "QUIT", 4) == 0) {
        printf("Server quitting...\n");
        exit(0);
    } else {
        printf("Unknown command: %s\n", stdin_command);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    static struct option long_options[] = {
        {"help",no_argument,0,'h'},
        {"TCP_PORT",optional_argument,0,'t'},
        {"UDP_PORT",optional_argument,0,'u'},
        {"payload",optional_argument,0,'p'},
        {0,0,0,0}
    };
    int long_index = 0;
    int TCP_port = 1234, UDP_port = 1234;

    while ((opt = getopt_long(argc, argv, "ht:u:p:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "Usage: %s [--TCP_port TCP_PORT] [--UDP_port UDP_port] [--payload PAYLOAD]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 't':
                TCP_port = atoi(optarg);
                break;
            case 'u':
                UDP_port = atoi(optarg);
                break;
            case 'p':
                pthread_mutex_lock(&mutex);
                strcpy(payload, optarg);
                pthread_mutex_unlock(&mutex);
                break;
            default:
                fprintf(stderr, "Usage: %s [--TCP_port TCP_PORT] [--UDP_port TCP_PORT] [--payload PAYLOAD]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }   

    printf("--Server started--\n");
    printf("Payload: %s\n", payload);
    printf("TCP_Port: %d\n", TCP_port);
    printf("UDP_Port: %d\n", UDP_port);

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create UDP and TCP socket
    int UDP_sockfd, TCP_sockfd;
    if ((UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP_Socket creation failed");
        return 1;
    }
    if ((TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP_Socket creation failed");
        return 1;
    }

    struct sockaddr_in myaddr_UDP,myaddr_TCP;
    // Seting up the UDP and TCP address structure
    myaddr_UDP.sin_family = AF_INET;
    myaddr_UDP.sin_port = htons(UDP_port);
    myaddr_UDP.sin_addr.s_addr = INADDR_ANY;
    myaddr_TCP.sin_family = AF_INET;
    myaddr_TCP.sin_port = htons(TCP_port);
    myaddr_TCP.sin_addr.s_addr = INADDR_ANY;
    // Binding the UDP and TCP socket to the specified address and port
    if (bind(UDP_sockfd, (struct sockaddr *)&myaddr_UDP, sizeof(myaddr_UDP)) < 0) {
        perror("UDP bind failed");
        close(UDP_sockfd);
        exit(EXIT_FAILURE);
    }
    if (bind(TCP_sockfd, (struct sockaddr *)&myaddr_TCP, sizeof(myaddr_TCP)) < 0) {
        perror("TCP bind failed");
        close(TCP_sockfd);
        exit(EXIT_FAILURE);
    }

    // Creating new thread IDs for registration and UDP
    pthread_t registration_thread, UDP_thread, TCP_thread;

    // Assign thread to listen for UDP and TCP messages
    if (pthread_create(&UDP_thread, NULL, listen_for_UDP_message, (void *)&UDP_sockfd) != 0) {
        perror("Failed to create UDP_thread");
        return 1;
    }
    if (pthread_create(&UDP_thread, NULL, listen_for_TCP_connections, (void *)&TCP_sockfd) != 0) {
        perror("Failed to create UDP_thread");
        return 1;
    }

    char stdin_command[MAX_COMMAND_SIZE];
    while(true){
        fgets(stdin_command, MAX_COMMAND_SIZE, stdin);
        handle_stdin_command(stdin_command);
    }

    // Program finish
    pthread_join(registration_thread, NULL);
    pthread_join(UDP_thread, NULL);
    pthread_join(TCP_thread, NULL);

    return 0;
}
