#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

// Configuration structure
typedef struct {
    char *host;
    int port;
    int num_connections;
    int messages_per_connection;
    int message_size;
} load_test_config_t;

// Thread data structure
typedef struct {
    int thread_id;
    load_test_config_t *config;
    int messages_sent;
    int messages_received;
    double total_response_time;
    int connection_errors;
} thread_data_t;

// Global statistics
typedef struct {
    pthread_mutex_t mutex;
    int total_messages_sent;
    int total_messages_received;
    int total_connections_failed;
    double total_time;
    struct timespec start_time;
    struct timespec end_time;
} global_stats_t;

global_stats_t g_stats = {PTHREAD_MUTEX_INITIALIZER, 0, 0, 0, 0.0};

// Function to get current time in milliseconds
double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Function to create a socket connection
int create_connection(const char *host, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed: %s\n", strerror(errno));
        return -1;
    }
    
    // Set socket options for reuse and timeout
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set connection timeout
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Try to parse as IP address first
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        // If not an IP address, resolve hostname
        server = gethostbyname(host);
        if (server == NULL) {
            fprintf(stderr, "Failed to resolve hostname %s: %s\n", host, hstrerror(h_errno));
            close(sockfd);
            return -1;
        }
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed: %s\n", host, port, strerror(errno));
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Function to send and receive messages
int send_receive_message(int sockfd, const char *message, char *response_buffer, int buffer_size) {
    int message_len = strlen(message);
    
    // Send message
    int bytes_sent = send(sockfd, message, message_len, 0);
    if (bytes_sent != message_len) {
        fprintf(stderr, "Send failed: expected %d bytes, sent %d bytes: %s\n", 
                message_len, bytes_sent, strerror(errno));
        return -1;
    }
    
    // Receive response
    int bytes_received = recv(sockfd, response_buffer, buffer_size - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            // Connection closed by server - this is expected for the Netty echo server
            return 0;
        } else {
            fprintf(stderr, "Receive failed: %s\n", strerror(errno));
            return -1;
        }
    }
    
    response_buffer[bytes_received] = '\0';
    return bytes_received;
}

// Thread function for load testing
void* load_test_thread(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    load_test_config_t *config = data->config;
    char message[256];
    char response[1024];
    
    // Create test message
    snprintf(message, sizeof(message), "Load test message from thread %d", data->thread_id);
    
    printf("Thread %d: Starting load test\n", data->thread_id);
    
    // Send messages - create new connection for each message since server closes after each response
    for (int i = 0; i < config->messages_per_connection; i++) {
        double start_time = get_time_ms();
        
        // Create new connection for each message
        int sockfd = create_connection(config->host, config->port);
        if (sockfd < 0) {
            data->connection_errors++;
            pthread_mutex_lock(&g_stats.mutex);
            g_stats.total_connections_failed++;
            pthread_mutex_unlock(&g_stats.mutex);
            printf("Thread %d: Failed to create connection for message %d\n", data->thread_id, i);
            continue;
        }
        
        // Create unique message for this iteration
        char unique_message[256];
        snprintf(unique_message, sizeof(unique_message), "%s #%d", message, i);
        
        // Send and receive message
        int result = send_receive_message(sockfd, unique_message, response, sizeof(response));
        
        // Close connection (server will close it anyway)
        close(sockfd);
        
        double end_time = get_time_ms();
        double response_time = end_time - start_time;
        
        if (result > 0) {
            data->messages_sent++;
            data->messages_received++;
            data->total_response_time += response_time;
            
            // Verify echo (basic check)
            if (strstr(response, unique_message) == NULL) {
                printf("Thread %d: Warning - Echo mismatch for message %d\n", 
                       data->thread_id, i);
            }
        } else {
            printf("Thread %d: Failed to send/receive message %d\n", data->thread_id, i);
        }
        
        // Small delay between messages to avoid overwhelming
        usleep(1000); // 1ms delay
    }
    
    // Update global statistics
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.total_messages_sent += data->messages_sent;
    g_stats.total_messages_received += data->messages_received;
    pthread_mutex_unlock(&g_stats.mutex);
    
    printf("Thread %d: Completed - Sent: %d, Received: %d, Avg Response: %.2fms\n",
           data->thread_id, data->messages_sent, data->messages_received,
           data->messages_received > 0 ? data->total_response_time / data->messages_received : 0.0);
    
    return NULL;
}

// Function to print usage
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h <host>       Server host (default: localhost)\n");
    printf("  -p <port>       Server port (default: 8888)\n");
    printf("  -c <connections> Number of concurrent connections (default: 10)\n");
    printf("  -m <messages>   Messages per connection (default: 5)\n");
    printf("  -s <size>       Message size in bytes (default: 64)\n");
    printf("  -?              Show this help\n");
    printf("\nExample: %s -h localhost -p 8080 -c 100 -m 10\n", program_name);
}

// Function to print test results
void print_results(load_test_config_t *config, thread_data_t *threads) {
    double total_time_ms = (g_stats.end_time.tv_sec - g_stats.start_time.tv_sec) * 1000.0 +
                          (g_stats.end_time.tv_nsec - g_stats.start_time.tv_nsec) / 1000000.0;
    
    double total_response_time = 0.0;
    int successful_threads = 0;
    
    for (int i = 0; i < config->num_connections; i++) {
        total_response_time += threads[i].total_response_time;
        if (threads[i].messages_received > 0) {
            successful_threads++;
        }
    }
    
    printf("\n=== Load Test Results ===\n");
    printf("Test Configuration:\n");
    printf("  Server: %s:%d\n", config->host, config->port);
    printf("  Concurrent connections: %d\n", config->num_connections);
    printf("  Messages per connection: %d\n", config->messages_per_connection);
    printf("  Expected total messages: %d\n", config->num_connections * config->messages_per_connection);
    
    printf("\nResults:\n");
    printf("  Total test time: %.2f ms (%.2f seconds)\n", total_time_ms, total_time_ms / 1000.0);
    printf("  Successful connections: %d\n", config->num_connections - g_stats.total_connections_failed);
    printf("  Failed connections: %d\n", g_stats.total_connections_failed);
    printf("  Messages sent: %d\n", g_stats.total_messages_sent);
    printf("  Messages received: %d\n", g_stats.total_messages_received);
    printf("  Success rate: %.2f%%\n", 
           g_stats.total_messages_sent > 0 ? 
           (g_stats.total_messages_received * 100.0 / g_stats.total_messages_sent) : 0.0);
    
    if (total_time_ms > 0) {
        printf("  Throughput: %.2f messages/second\n", 
               g_stats.total_messages_received * 1000.0 / total_time_ms);
        printf("  Average response time: %.2f ms\n", 
               g_stats.total_messages_received > 0 ? 
               total_response_time / g_stats.total_messages_received : 0.0);
    }
}

int main(int argc, char *argv[]) {
    // Default configuration
    load_test_config_t config = {
        .host = "localhost",
        .port = 8888,
        .num_connections = 10,
        .messages_per_connection = 5,
        .message_size = 64
    };
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "h:p:c:m:s:?")) != -1) {
        switch (opt) {
            case 'h':
                config.host = optarg;
                break;
            case 'p':
                config.port = atoi(optarg);
                break;
            case 'c':
                config.num_connections = atoi(optarg);
                break;
            case 'm':
                config.messages_per_connection = atoi(optarg);
                break;
            case 's':
                config.message_size = atoi(optarg);
                break;
            case '?':
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Validate configuration
    if (config.num_connections <= 0 || config.num_connections > 1000) {
        printf("Error: Number of connections must be between 1 and 1000\n");
        return 1;
    }
    
    if (config.messages_per_connection <= 0 || config.messages_per_connection > 1000) {
        printf("Error: Messages per connection must be between 1 and 1000\n");
        return 1;
    }
    
    printf("Starting load test with %d connections, %d messages each\n",
           config.num_connections, config.messages_per_connection);
    printf("Target server: %s:%d\n", config.host, config.port);
    
    // Test connection first
    printf("Testing connection to %s:%d...\n", config.host, config.port);
    int test_sock = create_connection(config.host, config.port);
    if (test_sock < 0) {
        printf("ERROR: Cannot connect to server %s:%d\n", config.host, config.port);
        printf("Make sure the Netty echo server is running with: java EchoServer %d\n", config.port);
        return 1;
    }
    
    // Test a simple echo
    char test_msg[] = "Test connection";
    char test_response[256];
    int result = send_receive_message(test_sock, test_msg, test_response, sizeof(test_response));
    close(test_sock);
    
    if (result <= 0) {
        printf("ERROR: Failed to receive echo response from server\n");
        return 1;
    }
    
    printf("Connection test successful! Server echoed: %s\n", test_response);
    printf("Proceeding with load test...\n\n");
    
    // Allocate thread data
    thread_data_t *threads = calloc(config.num_connections, sizeof(thread_data_t));
    pthread_t *thread_ids = calloc(config.num_connections, sizeof(pthread_t));
    
    if (!threads || !thread_ids) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }
    
    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &g_stats.start_time);
    
    // Create threads
    for (int i = 0; i < config.num_connections; i++) {
        threads[i].thread_id = i;
        threads[i].config = &config;
        threads[i].messages_sent = 0;
        threads[i].messages_received = 0;
        threads[i].total_response_time = 0.0;
        threads[i].connection_errors = 0;
        
        if (pthread_create(&thread_ids[i], NULL, load_test_thread, &threads[i]) != 0) {
            printf("Error: Failed to create thread %d\n", i);
            return 1;
        }
        
        // Small delay between thread creation to avoid overwhelming
        if (i % 10 == 0 && i > 0) {
            usleep(10000); // 10ms delay every 10 threads
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < config.num_connections; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    
    // Record end time
    clock_gettime(CLOCK_MONOTONIC, &g_stats.end_time);
    
    // Print results
    print_results(&config, threads);
    
    // Cleanup
    free(threads);
    free(thread_ids);
    
    return 0;
}

/*
To compile and run:

gcc -o load_test_client load_test_client.c -lpthread

# Basic test
./load_test_client

# Custom test with 100 connections, 10 messages each
./load_test_client -c 100 -m 10

# Test against specific server
./load_test_client -h 192.168.1.100 -p 8888 -c 50 -m 20

# Heavy load test
./load_test_client -c 500 -m 5
*/
