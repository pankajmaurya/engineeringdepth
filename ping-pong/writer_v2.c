#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_KEY 1234
#define SHM_SIZE 1024

struct shared_data {
    int counter;
    char message[256];
    int sequence_num;  // Better synchronization
    int finished;
};

int main() {
    int shmid;
    struct shared_data *shm_ptr;
    
    // Create shared memory segment
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }
    
    // Attach to shared memory
    shm_ptr = (struct shared_data *)shmat(shmid, NULL, 0);
    if (shm_ptr == (struct shared_data *)(-1)) {
        perror("shmat failed");
        exit(1);
    }
    
    printf("Writer: Connected to shared memory\n");
    
    // Initialize data
    shm_ptr->counter = 0;
    shm_ptr->sequence_num = 0;
    shm_ptr->finished = 0;
    
    // Write data to shared memory
    for (int i = 1; i <= 10; i++) {
        shm_ptr->counter = i;
        snprintf(shm_ptr->message, sizeof(shm_ptr->message), 
                "Hello from writer, iteration %d", i);
        shm_ptr->sequence_num = i;  // Increment sequence number
        
        printf("Writer: Wrote counter=%d, message='%s', seq=%d\n", 
               shm_ptr->counter, shm_ptr->message, shm_ptr->sequence_num);
        
        sleep(2);  // Wait 2 seconds between writes
    }
    
    // Signal end of data
    shm_ptr->finished = 1;
    printf("Writer: Finished writing data\n");
    
    // Keep the writer alive for a bit so reader can finish
    sleep(2);
    
    // Detach from shared memory
    if (shmdt(shm_ptr) < 0) {
        perror("shmdt failed");
        exit(1);
    }
    
    printf("Writer: Detached from shared memory\n");
    return 0;
}
