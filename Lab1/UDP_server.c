#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
    int opt;
    static struct option long_options[] = {
        {"help",no_argument,0,'h'},
        {"port",optional_argument,0,'o'},
        {"payload",optional_argument,0,'a'},
        {0,0,0,0}
    };
    int long_index = 0;
    char *payload = "";
    int port = 1234;

    while ((opt = getopt_long(argc, argv, "ho::a::", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "Usage: %s [--port PORT] [--payload PAYLOAD]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'o':
                port = atoi(optarg);
                break;
            case 'a':
                payload = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [--port PORT] [--payload PAYLOAD]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (port == 0) {
        fprintf(stderr, "Error: --port requires an integer value.\n");
        fprintf(stderr, "Usage: %s [--port PORT] [--payload PAYLOAD]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("--Server started--\n");
    printf("Payload: %s\n", payload);
    printf("Port: %d\n", port);

    return 0;
}
