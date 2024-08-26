#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#define MAX_MESSAGE_SIZE 1024
#define MAX_COMMAND_SIZE 50 
#define MAX_BOT_COUNT 25
#define UDP_PORT 5555
#define MAX_TCP_THREAD_COUNT 25
#define BACKLOG 5
#define BOT_INFO_SIZE 128

pthread_mutex_t mutex;
struct sockaddr_in bots[MAX_BOT_COUNT];

struct MSG {
    char command;
    char IP1[INET_ADDRSTRLEN];
    char PORT1[22];
    char IP2[INET_ADDRSTRLEN];
    char PORT2[22];
    char IP3[INET_ADDRSTRLEN];
    char PORT3[22];
    char IP4[INET_ADDRSTRLEN];
    char PORT4[22];
    char IP5[INET_ADDRSTRLEN];
    char PORT5[22];
    char IP6[INET_ADDRSTRLEN];
    char PORT6[22];
    char IP7[INET_ADDRSTRLEN];
    char PORT7[22];
    char IP8[INET_ADDRSTRLEN];
    char PORT8[22];
    char IP9[INET_ADDRSTRLEN];
    char PORT9[22];
    char IP10[INET_ADDRSTRLEN];
    char PORT10[22];
    char IP11[INET_ADDRSTRLEN];
    char PORT11[22];
    char IP12[INET_ADDRSTRLEN];
    char PORT12[22];
    char IP13[INET_ADDRSTRLEN];
    char PORT13[22];
    char IP14[INET_ADDRSTRLEN];
    char PORT14[22];
    char IP15[INET_ADDRSTRLEN];
    char PORT15[22];
    char IP16[INET_ADDRSTRLEN];
    char PORT16[22];
    char IP17[INET_ADDRSTRLEN];
    char PORT17[22];
    char IP18[INET_ADDRSTRLEN];
    char PORT18[22];
    char IP19[INET_ADDRSTRLEN];
    char PORT19[22];
    char IP20[INET_ADDRSTRLEN];
    char PORT20[22];
};

struct data_to_handle_TCP {
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len;
    int TCP_socket, UDP_socket, incoming_socket;
};

struct socket_data{
    int TCP_socket, UDP_socket;
};

// Function to prepare the help message string
char* get_help_message() {
    // Allocate memory for the message
    char *message = malloc(1024 * sizeof(char)); // Adjust size as needed
    if (message == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Create the help message
    snprintf(message, 1024, 
        "Podrzane su naredbe:\n"
        "pt... bot klijentima salje poruku PROG_TCP\n"
        "    struct MSG:1 10.0.0.20 1234\n"
        "ptl... bot klijentima salje poruku PROG_TCP\n"
        "    struct MSG:1 127.0.0.1 1234\n"
        "pu... bot klijentima salje poruku PROG_UDP\n"
        "    struct MSG:2 10.0.0.20 1234\n"
        "pul... bot klijentima salje poruku PROG_UDP\n"
        "    struct MSG:2 127.0.0.1 1234\n"
        "r ... bot klijentima salje poruku RUN s adresama iz ifconfig\n"
        "    struct MSG:3 127.0.0.1 watt localhost 6789\n"
        "r2... bot klijentima salje poruku RUN s nekim adresama\n"
        "    struct MSG:3 20.0.0.11 1111 20.0.0.12 2222 20.0.0.13 hostmon\n"
        "s ... bot klijentima salje poruku STOP ('4')\n"
        "l ... lokalni ispis adresa bot klijenata\n"
        "n ... salje poruku: 'NEPOZNATA'\n"
        "q ... bot klijentima salje poruku QUIT ('0') i zavrssava s radom\n"
        "h ... ispis naredbi\n"
    );

    return message;
}

// Function to prepare the bot information string
char *get_bot_info() {
    // Allocate memory for the buffer
    char *buffer = malloc(MAX_BOT_COUNT*BOT_INFO_SIZE);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&mutex);

    // Initialize the buffer with an empty string
    buffer[0] = '\0';
    // Iterate through the array and prepare the bot information
    for (int i = 0; i < MAX_BOT_COUNT; i++) {
        // Check if the bot is initialized
        if (bots[i].sin_family == AF_INET) {
            // Buffer to store IP address as a string
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(bots[i].sin_addr), ip, INET_ADDRSTRLEN);
            int port = ntohs(bots[i].sin_port);

            // Prepare the bot info and append it to the buffer
            char bot_info[BOT_INFO_SIZE];
            snprintf(bot_info, sizeof(bot_info), "Bot %d: IP: %s, Port: %d\n", i, ip, port);

            // Check if there is enough space in the buffer
            if (strlen(buffer) + strlen(bot_info) < MAX_BOT_COUNT*BOT_INFO_SIZE) {
                strncat(buffer, bot_info, strlen(bot_info));
            } else {
                perror("Not enough space for more bots");
            }
        } else {
            break;
        }
    }

    pthread_mutex_unlock(&mutex);

    // Return the dynamically allocated buffer
    return buffer;
}

// Send "NEPOZNATA" message to bots
void send_nepoznata_to_clients(char *message, int UDP_socket){

    pthread_mutex_lock(&mutex);
    // Iterate through the array to get all bots
    for (int i = 0; i < MAX_BOT_COUNT; i++) {
        // Check if the bot is initialized
        if (bots[i].sin_family == AF_INET) {
            // For existing bots
            if (sendto(UDP_socket, message, strlen(message), 0, (const struct sockaddr *)&bots[i], sizeof(bots[i])) < 0) {
                perror("Send message failed");
            } else {
                // Usually use inet_ntop() instead of inet_ntoa(), but for simplicity we use inet_ntoa()
                printf("Message sent to %s:%d\n", inet_ntoa(bots->sin_addr), ntohs(bots[i].sin_port));
            }
        }
        else{
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    free(message);
}

// Send message to bots
void send_msg_to_bots(struct MSG *msg, int UDP_socket){

    pthread_mutex_lock(&mutex);
    // Iterate through the array to get all bots
    for (int i = 0; i < MAX_BOT_COUNT; i++) {
        // Check if the bot is initialized
        if (bots[i].sin_family == AF_INET) {
            // For existing bots
            if (sendto(UDP_socket, msg, sizeof(struct MSG), 0, (const struct sockaddr *)&bots[i], sizeof(bots[i])) < 0) {
                perror("Send message failed");
            } else {
                // Usually use inet_ntop() instead of inet_ntoa(), but for simplicity we use inet_ntoa()
                printf("Message sent to %s:%d\n", inet_ntoa(bots[i].sin_addr), ntohs(bots[i].sin_port));
            }
        }
        else{
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    free(msg);
}

// Listen for UDP messages
void *listen_for_UDP_message(void *arg){    

    // Dereference the argument to get the socket descriptor
    int UDP_socket = *(int *)arg;

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

            if(strcmp(UDP_message_buffer, "REG\n") == 0){
                pthread_mutex_lock(&mutex);
                // Iterate through the array and find the first free spot for bot
                for (int i = 0; i < MAX_BOT_COUNT; i++) {
                    // Check if the bot is initialized
                    if (bots[i].sin_family == AF_INET) {
                        // Bot is initialized and we skip it
                        continue;
                    }
                    else{
                        bots[i].sin_family = incoming_addr.sin_family;
                        bots[i].sin_addr = incoming_addr.sin_addr;
                        bots[i].sin_port = incoming_addr.sin_port;
                        break;
                    }
                }
                printf("New bot added: %s %d\n", inet_ntoa(bots->sin_addr), ntohs(bots->sin_port));
                pthread_mutex_unlock(&mutex);
            }
            else{
                printf("Recieved message: \"%s\"\n", UDP_message_buffer);
            }
        }
    }
}

// Handle TCP connection
void *handle_TCP_connection(void *arg) {
    
    // Dereference the argument to get the struct in which the data is stored
    struct data_to_handle_TCP *handle_TCP_data = (struct data_to_handle_TCP *)arg;
    // Initialize the buffer where request will be stored
    char buffer[MAX_MESSAGE_SIZE] = {0};
    int bytes_read;

    // Read the HTTP request
    bytes_read = read(handle_TCP_data->incoming_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("Error reading from socket");
        close(handle_TCP_data->incoming_socket);
        free(handle_TCP_data);
        pthread_exit(NULL);
    }

    // Null-terminate the request
    buffer[bytes_read] = '\0';
    printf("Received HTTP request (%s:%d):\n%s\n", inet_ntoa(handle_TCP_data->incoming_addr.sin_addr), ntohs(handle_TCP_data->incoming_addr.sin_port), buffer);

    // Parse the HTTP method
    char method[8], path[1024], version[16];
    // Try to scan for typical format (METHOD PATH VERSION)
    int parsed_items = sscanf(buffer, "%s %s %s", method, path, version);
    // Make sure request is in proper form
    if (parsed_items != 3) {
        // Handle the error: malformed request
        fprintf(stderr, "Error: Malformed HTTP request\n");
        pthread_exit(NULL);
    }

    // Handle only GET requests
    if (strcmp(method, "GET") != 0) {
        // Request being sent if method is not GET
        const char *response = "HTTP/1.0 405 Method Not Allowed\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 23\r\n"
                               "\r\n"
                               "405 Method Not Allowed\n";
        send(handle_TCP_data->incoming_socket, response, strlen(response), 0);
    } else {
        // Serve only files in the current directory with allowed extensions
        char *allowed_extensions[] = {".html", ".txt", ".gif", ".jpg", ".pdf"};
        int allowed = 0;
        // Dividing by 8 because method size is 8
        for (int i = 0; i < sizeof(allowed_extensions) / 8; i++) {
            // Checks if string contains some extension
            if (strstr(path, allowed_extensions[i]) != NULL) {
                allowed = 1;
                break;
            }
        }

        // Alocate memory for msg structure that will be filled by web
        struct MSG *msgWeb = malloc(sizeof(struct MSG));
        if (msgWeb == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        // Initialize all bytes of the structure to zero
        memset(msgWeb, 0, sizeof(struct MSG));

        // Check for url calls
        if(strcmp(path, "/bot/prog_tcp") == 0){
            msgWeb->command = '1';
            strncpy(msgWeb->IP1, "10.0.0.20", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "1234", sizeof(msgWeb->PORT1) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/prog_tcp_localhost") == 0){
            msgWeb->command = '1';
            strncpy(msgWeb->IP1, "127.0.0.1", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "1234", sizeof(msgWeb->PORT1) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/prog_udp") == 0){
            msgWeb->command = '2';
            strncpy(msgWeb->IP1, "10.0.0.20", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "1234", sizeof(msgWeb->PORT1) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/prog_udp_localhost") == 0){
            msgWeb->command = '2';
            strncpy(msgWeb->IP1, "127.0.0.1", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "1234", sizeof(msgWeb->PORT1) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/run") == 0){
            msgWeb->command = '3';
            strncpy(msgWeb->IP1, "127.0.0.1", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "vat", sizeof(msgWeb->PORT1) - 1);
            strncpy(msgWeb->IP2, "localhost", sizeof(msgWeb->IP2) - 1);
            strncpy(msgWeb->PORT2, "6789", sizeof(msgWeb->PORT2) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/run2") == 0){
            msgWeb->command = '3';
            strncpy(msgWeb->IP1, "20.0.0.11", sizeof(msgWeb->IP1) - 1);
            strncpy(msgWeb->PORT1, "1111", sizeof(msgWeb->PORT1) - 1);
            strncpy(msgWeb->IP2, "20.0.0.12", sizeof(msgWeb->IP2) - 1);
            strncpy(msgWeb->PORT2, "2222", sizeof(msgWeb->PORT2) - 1);
            strncpy(msgWeb->IP3, "20.0.0.13", sizeof(msgWeb->IP3) - 1);
            strncpy(msgWeb->PORT3, "hostmon", sizeof(msgWeb->PORT3) - 1);
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/stop") == 0){
            msgWeb->command = '4';
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
        }
        else if(strcmp(path, "/bot/list") == 0){
            // Generate the bot information
            char *bot_info = get_bot_info();

            // Create HTTP response headers
            char headers[MAX_MESSAGE_SIZE];
            snprintf(headers, sizeof(headers),
                    "HTTP/1.0 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %zu\r\n"
                    "\r\n", strlen(bot_info));

            // Send headers
            send(handle_TCP_data->incoming_socket, headers, strlen(headers), 0);
            // Send the bot information
            send(handle_TCP_data->incoming_socket, bot_info, strlen(bot_info), 0);

            // Free the allocated memory for bot info
            free(bot_info);
        }
        else if(strcmp(path, "/bot/quit") == 0){
            msgWeb->command = '0';
            send_msg_to_bots(msgWeb, handle_TCP_data->UDP_socket);
            exit(0);
        }
        else if (!allowed) {
            // If extension is not found return the forbidden message
            const char *response = "HTTP/1.0 403 Forbidden\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 10\r\n"
                                   "\r\n"
                                   "403 Forbidden\n";
            send(handle_TCP_data->incoming_socket, response, strlen(response), 0);

        } else {

            // Making the path go to current working directory by adding .
            char file_path[1025];
            snprintf(file_path, sizeof(file_path), ".%s", path);

            // Open the requested file
            FILE *file = fopen(file_path, "rb");

            // If file does not exist send not found message
            if (!file) {
                const char *response = "HTTP/1.0 404 Not Found\r\n"
                                       "Content-Type: text/plain\r\n"
                                       "Content-Length: 15\r\n"
                                       "\r\n"
                                       "404 Not Found\n";
                send(handle_TCP_data->incoming_socket, response, strlen(response), 0);

            } else {
                // Send HTTP response headers
                const char *headers = "HTTP/1.0 200 OK\r\n"
                                      "Content-Type: application/octet-stream\r\n"
                                      "\r\n";
                send(handle_TCP_data->incoming_socket, headers, strlen(headers), 0);

                // Send the file content
                char file_buffer[1024];
                int bytes;
                while ((bytes = fread(file_buffer, sizeof(char), sizeof(file_buffer), file)) > 0) {
                    send(handle_TCP_data->incoming_socket, file_buffer, bytes, 0);
                }
                fclose(file);
            }
        }
    }

    // Close the connection and free memory
    close(handle_TCP_data->incoming_socket);
    free(handle_TCP_data);
    pthread_exit(NULL);
}

// Listen for TCP connections
void *listen_for_TCP_connections(void *arg){    

    // Dereference the argument to get the socket descriptors
    struct socket_data *sock_data = (struct socket_data *)arg;

    // Keep track of active threads
    pthread_t TCP_threads[MAX_TCP_THREAD_COUNT];
    int TCP_thread_id = 0;

    // Initialize the struct and socket where the incoming address will be stored
    int incoming_socket;
    struct sockaddr_in incoming_addr;
    socklen_t incoming_addr_len = sizeof(incoming_addr);

    // Listening for incoming connections
    if (listen(sock_data->TCP_socket, BACKLOG) < 0) {
        perror("Listen failed");
        close(sock_data->TCP_socket);
    }

    while(true){

        // Accept a new connection
        if ((incoming_socket = accept(sock_data->TCP_socket, (struct sockaddr *)&incoming_addr, &incoming_addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for struct because of potential race conditions and asign all variables that will be passed to the new thread
        struct data_to_handle_TCP *handle_TCP_data = malloc(sizeof(struct data_to_handle_TCP));
        if (handle_TCP_data == NULL) {
            perror("Failed to allocate memory");
            continue;
        }
        handle_TCP_data->incoming_addr = incoming_addr;
        handle_TCP_data->incoming_addr_len = incoming_addr_len;
        handle_TCP_data->incoming_socket = incoming_socket;
        handle_TCP_data->TCP_socket = sock_data->TCP_socket;
        handle_TCP_data->UDP_socket = sock_data->UDP_socket;
        // Create a thread to handle the TCP connection
        if (pthread_create(&TCP_threads[TCP_thread_id], NULL, handle_TCP_connection, (void *)handle_TCP_data) != 0) {
            perror("Failed to create thread");
            free(handle_TCP_data);
            continue;
        }
        // Increase thread id for the next iteration
        TCP_thread_id++;   
    }
}

// Handle fetched stdin commands
void handle_stdin_command(char *stdin_command, int UDP_socket) {
    // Memory alocation for msg structure
    struct MSG *msg = malloc(sizeof(struct MSG));
    if (msg == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Trailing newline character removal
    stdin_command[strcspn(stdin_command, "\n")] = '\0';

    // Initialize all bytes of the structure to zero
    memset(msg, 0, sizeof(struct MSG));

    // Handle commands
    if (strcmp(stdin_command, "pt") == 0) {
        msg->command = '1';
        // Copy the IP address and port into msg
        strncpy(msg->IP1, "10.0.0.20", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "1234", sizeof(msg->PORT1) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "ptl") == 0) {
        msg->command = '1';
        strncpy(msg->IP1, "127.0.0.1", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "1234", sizeof(msg->PORT1) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "pu") == 0) {
        msg->command = '2';
        strncpy(msg->IP1, "10.0.0.20", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "1234", sizeof(msg->PORT1) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "pul") == 0) {
        msg->command = '2';
        strncpy(msg->IP1, "127.0.0.1", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "1234", sizeof(msg->PORT1) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "r") == 0) {
        msg->command = '3';
        strncpy(msg->IP1, "127.0.0.1", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "vat", sizeof(msg->PORT1) - 1);
        strncpy(msg->IP2, "localhost", sizeof(msg->IP2) - 1);
        strncpy(msg->PORT2, "6789", sizeof(msg->PORT2) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "r2") == 0) {
        msg->command = '3';
        strncpy(msg->IP1, "20.0.0.11", sizeof(msg->IP1) - 1);
        strncpy(msg->PORT1, "1111", sizeof(msg->PORT1) - 1);
        strncpy(msg->IP2, "20.0.0.12", sizeof(msg->IP2) - 1);
        strncpy(msg->PORT2, "2222", sizeof(msg->PORT2) - 1);
        strncpy(msg->IP3, "20.0.0.13", sizeof(msg->IP3) - 1);
        strncpy(msg->PORT3, "hostmon", sizeof(msg->PORT3) - 1);
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "s") == 0) {
        msg->command = '4';
        send_msg_to_bots(msg, UDP_socket);

    } else if (strcmp(stdin_command, "l") == 0) {
        // Generate the bot info
        char *bot_info = get_bot_info();

        // Print the help message
        printf("%s", bot_info);

        // Free the allocated memory
        free(bot_info); 

    } else if (strcmp(stdin_command, "n") == 0) {
        char *message = "NEPOZNATA\n"; 
        send_nepoznata_to_clients(message, UDP_socket);

    } else if (strcmp(stdin_command, "q") == 0) {
        msg->command = '0';
        send_msg_to_bots(msg, UDP_socket);
        free(msg);
        exit(0);

    } else if (strncmp(stdin_command, "h", 1) == 0) {
       // Generate the help message
        char *help = get_help_message();

        // Print the help message
        printf("%s", help);

        // Free the allocated memory
        free(help); 
    } else {
        printf("Unknown command: %s\n", stdin_command);
        char *help = get_help_message();
        printf("%s", help);
        free(help);
    }
}

int main(int argc, char *argv[]) {
    // Options for running program initialization
    int opt;
    static struct option long_options[] = {
        {"help",no_argument,0,'h'},
        {"TCP_PORT",optional_argument,0,'t'},
        {0,0,0,0}
    };
    int long_index = 0;

    // Tcp port initialization
    int TCP_port = 5555;

    // Input options setup
    while ((opt = getopt_long(argc, argv, "ht:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "Usage: %s [--TCP_port TCP_PORT]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 't':
                TCP_port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [--TCP_port TCP_PORT]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }   

    // Start message
    printf("--CandC server started--\n");
    printf("TCP_Port: %d\n", TCP_port);
    printf("UDP_Port: %d\n", UDP_PORT);

    // Mutex initialization
    pthread_mutex_init(&mutex, NULL);

    // UDP and TCP socket creation
    int UDP_sockfd, TCP_sockfd;
    if ((UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        return 1;
    }
    if ((TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP socket creation failed");
        return 1;
    }

    // Sockaddr_in structure initialization for UDP and TCP
    struct sockaddr_in myaddr_UDP,myaddr_TCP;
    myaddr_UDP.sin_family = AF_INET;
    myaddr_UDP.sin_port = htons(UDP_PORT);
    myaddr_UDP.sin_addr.s_addr = INADDR_ANY;
    myaddr_TCP.sin_family = AF_INET;
    myaddr_TCP.sin_port = htons(TCP_port);
    myaddr_TCP.sin_addr.s_addr = INADDR_ANY;

    // UDP and TCP socket bind to the specified address and port
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

    // New thread IDs for UDP and TCP
    pthread_t UDP_thread, TCP_thread;

    // Initialization of data that is needed for a thread
    struct socket_data sock_data;
    sock_data.TCP_socket = TCP_sockfd;
    sock_data.UDP_socket = UDP_sockfd;

    // Thread creation that will listen for UDP messages and TCP connections
    if (pthread_create(&UDP_thread, NULL, listen_for_UDP_message, (void *)&UDP_sockfd) != 0) {
        perror("Failed to create UDP_thread");
        return 1;
    }
    if (pthread_create(&TCP_thread, NULL, listen_for_TCP_connections, (void *)&sock_data) != 0) {
        perror("Failed to create UDP_thread");
        return 1;
    }

    // Fetching commands from stdin
    char stdin_command[MAX_COMMAND_SIZE];
    while(true){
        fgets(stdin_command, MAX_COMMAND_SIZE, stdin);
        handle_stdin_command(stdin_command,UDP_sockfd);
    }

    // Program finish (just for safety, the program should not reach this far usually)
    pthread_join(UDP_thread, NULL);
    pthread_join(TCP_thread, NULL);

    return 0;
}
