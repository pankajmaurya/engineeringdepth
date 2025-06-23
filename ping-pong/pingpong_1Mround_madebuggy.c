#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#define SHM_KEY 5678
#define SHM_SIZE 1024
#define LOG_INTERVAL 1000000

struct shared_state {
    int ping_ready;        // 1 when ping is sent, 0 otherwise
    int pong_ready;        // 1 when pong is sent, 0 otherwise
    int ping_alive;        // heartbeat for ping server
    int pong_alive;        // heartbeat for pong server
    long ping_timestamp;   // timestamp when ping was sent (microseconds)
    long pong_timestamp;   // timestamp when pong was received (microseconds)
    int round_count;       // total completed round trips
    int reset_flag;        // flag to reset the game
};

static volatile int running = 1;
static int shmid;
static struct shared_state *shm_ptr;

void cleanup_and_exit(int sig) {
    printf("\nReceived signal %d, cleaning up...\n", sig);
    running = 0;
}

long get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

void ping_server() {
    printf("Starting PING server...\n");
    
    long total_latency = 0;
    int local_round_count = 0;
    long start_time = get_timestamp_us();
    
    // Initialize as ping server
    shm_ptr->ping_alive = 1;
    shm_ptr->ping_ready = 0;
    shm_ptr->pong_ready = 0;
    shm_ptr->round_count = 0;
    
    while (running) {
        // Update heartbeat
        shm_ptr->ping_alive = 1;
        
        // Wait for pong server to be alive
        if (!shm_ptr->pong_alive) {
            printf("Waiting for PONG server to join...\n");
            while (!shm_ptr->pong_alive && running) {
                shm_ptr->ping_alive = 1; // Keep updating heartbeat
            }
            if (!running) break;
            printf("PONG server joined! Starting ping-pong...\n");
        }
        
        // Send PING
        shm_ptr->ping_timestamp = get_timestamp_us();
        shm_ptr->ping_ready = 1;
        shm_ptr->pong_ready = 0;
        
        // Wait for PONG response
        while (shm_ptr->ping_ready == 1 && running && shm_ptr->pong_alive) {
            shm_ptr->ping_alive = 1; // Update heartbeat
        }
        
        // Check if pong server died
        if (!shm_ptr->pong_alive) {
            printf("PONG server died! Waiting for it to restart...\n");
            continue;
        }
        
        if (!running) break;
        
        // PONG received, calculate latency
        if (shm_ptr->pong_ready == 1) {
            long latency = shm_ptr->pong_timestamp - shm_ptr->ping_timestamp;
            total_latency += latency;
            local_round_count++;
            shm_ptr->round_count++;
            
            // Reset pong_ready for next round
            shm_ptr->pong_ready = 0;
            
            // Log every 1 million rounds
            if (local_round_count % LOG_INTERVAL == 0) {
                double avg_latency = (double)total_latency / LOG_INTERVAL;
                long elapsed = get_timestamp_us() - start_time;
                double throughput = (double)LOG_INTERVAL * 1000000.0 / elapsed;
                
                printf("PING: Completed %dM rounds, Avg latency: %.2f μs, Throughput: %.2f rounds/sec\n", 
                       local_round_count / 1000000, avg_latency, throughput);
                
                total_latency = 0;
                start_time = get_timestamp_us();
            }
        }
        
        // Small delay before next ping - REMOVED FOR MAX PERFORMANCE
    }
    
    printf("PING server shutting down after %d rounds\n", local_round_count);
}

void pong_server() {
    printf("Starting PONG server...\n");
    
    int local_round_count = 0;
    
    // Mark pong server as alive
    shm_ptr->pong_alive = 1;
    
    while (running) {
        // Update heartbeat
        shm_ptr->pong_alive = 1;
        
        // Wait for ping server to be alive
        if (!shm_ptr->ping_alive) {
            printf("Waiting for PING server to join...\n");
            while (!shm_ptr->ping_alive && running) {
                shm_ptr->pong_alive = 1; // Keep updating heartbeat
            }
            if (!running) break;
            printf("PING server joined! Ready for ping-pong...\n");
        }
        
        // Wait for PING
        while (shm_ptr->ping_ready == 0 && running && shm_ptr->ping_alive) {
            shm_ptr->pong_alive = 1; // Update heartbeat
        }
        
        // Check if ping server died
        if (!shm_ptr->ping_alive) {
            printf("PING server died! Waiting for it to restart...\n");
            continue;
        }
        
        if (!running) break;
        
        // PING received, send PONG
        if (shm_ptr->ping_ready == 1) {
            // SIMULATE BUG: Crash when ping count reaches 1 billion
            if (shm_ptr->round_count >= 1000000000) {
                printf("PONG: FATAL ERROR - Simulated crash at 1 billion pings!\n");
                printf("PONG: Process crashing due to simulated bug...\n");
                fflush(stdout);
                exit(1); // Simulate crash
            }
            
            shm_ptr->pong_timestamp = get_timestamp_us();
            shm_ptr->ping_ready = 0;  // Clear ping flag
            shm_ptr->pong_ready = 1;  // Set pong flag
            local_round_count++;
            
            // Log every 1 million rounds
            if (local_round_count % LOG_INTERVAL == 0) {
                printf("PONG: Responded to %dM pings (Total rounds: %d)\n", 
                       local_round_count / 1000000, shm_ptr->round_count);
            }
        }
        
        // No delay - MAX PERFORMANCE
    }
    
    printf("PONG server shutting down after %d responses\n", local_round_count);
}

void monitor_other_server(int is_ping_server) {
    // Simple heartbeat monitor - reset the other server's alive flag periodically
    // This simulates detecting if the other process has died
    static long last_check = 0;
    long now = get_timestamp_us();
    
    if (now - last_check > 1000000) { // Check every second
        if (is_ping_server) {
            // Ping server checks if pong server is responding
            static int last_pong_alive = 1;
            if (last_pong_alive == shm_ptr->pong_alive) {
                shm_ptr->pong_alive = 0; // Mark as potentially dead
            }
            last_pong_alive = shm_ptr->pong_alive;
        } else {
            // Pong server checks if ping server is responding
            static int last_ping_alive = 1;
            if (last_ping_alive == shm_ptr->ping_alive) {
                shm_ptr->ping_alive = 0; // Mark as potentially dead
            }
            last_ping_alive = shm_ptr->ping_alive;
        }
        last_check = now;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ping|pong>\n", argv[0]);
        printf("  ping - Run as PING server (initiates ping-pong)\n");
        printf("  pong - Run as PONG server (responds to pings)\n");
        exit(1);
    }
    
    int is_ping_server = (strcmp(argv[1], "ping") == 0);
    int is_pong_server = (strcmp(argv[1], "pong") == 0);
    
    if (!is_ping_server && !is_pong_server) {
        printf("Invalid mode. Use 'ping' or 'pong'\n");
        exit(1);
    }
    
    // Set up signal handlers
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    // Create or get shared memory
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }
    
    // Attach to shared memory
    shm_ptr = (struct shared_state *)shmat(shmid, NULL, 0);
    if (shm_ptr == (struct shared_state *)(-1)) {
        perror("shmat failed");
        exit(1);
    }
    
    // Initialize shared memory if this is the first process
    static int initialized = 0;
    if (!initialized) {
        memset(shm_ptr, 0, sizeof(struct shared_state));
        initialized = 1;
    }
    
    printf("Connected to shared memory, running in %s mode\n", argv[1]);
    
    // Run appropriate server
    if (is_ping_server) {
        ping_server();
    } else {
        pong_server();
    }
    
    // Cleanup
    if (is_ping_server) {
        shm_ptr->ping_alive = 0;
    } else {
        shm_ptr->pong_alive = 0;
    }
    
    // Detach from shared memory
    if (shmdt(shm_ptr) < 0) {
        perror("shmdt failed");
    }
    
    // Remove shared memory if we're the last one
    if (shmctl(shmid, IPC_RMID, NULL) == 0) {
        printf("Shared memory cleaned up\n");
    }
    
    return 0;
}
