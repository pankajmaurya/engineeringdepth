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
    int sequence_num;
    int finished;
};

int main() {
    int shmid;
    struct shared_data *shm_ptr;
    int last_sequence = 0;
    
    // Get existing shared memory segment
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("shmget failed - make sure writer is running first");
        exit(1);
    }
    
    // Attach to shared memory
    shm_ptr = (struct shared_data *)shmat(shmid, NULL, 0);
    if (shm_ptr == (struct shared_data *)(-1)) {
        perror("shmat failed");
        exit(1);
    }
    
    printf("Reader: Connected to shared memory\n");
    
    // Read data from shared memory
    while (!shm_ptr->finished) {
        // Check if there's new data
        if (shm_ptr->sequence_num > last_sequence) {
            printf("Reader: Read counter=%d, message='%s', seq=%d\n", 
                   shm_ptr->counter, shm_ptr->message, shm_ptr->sequence_num);
            last_sequence = shm_ptr->sequence_num;
        }
        
        usleep(500000);  // Sleep for 500ms
    }
    
    printf("Reader: Writer finished, exiting\n");
    
    // Detach from shared memory
    if (shmdt(shm_ptr) < 0) {
        perror("shmdt failed");
        exit(1);
    }
    
    // Remove shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("shmctl failed");
        exit(1);
    }
    
    printf("Reader: Detached and removed shared memory\n");
    return 0;
}
