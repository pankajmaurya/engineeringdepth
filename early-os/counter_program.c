#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// System call interface
void sys_write(const char* message) {
    printf("COUNTER PROGRAM: %s", message);
    fflush(stdout);
}

void sys_exit() {
    printf("COUNTER PROGRAM: Requesting program termination\n");
    exit(0);
}

int main(int argc, char* argv[]) {
    int program_id = 0;
    
    // Get program ID from command line
    if (argc > 1) {
        program_id = atoi(argv[1]);
    }
    
    char welcome[256];
    snprintf(welcome, sizeof(welcome), 
             "Counter program started (ID: %d)\n", program_id);
    sys_write(welcome);
    
    // Ask user for counting parameters
    char input[256];
    int max_count = 10;
    int delay = 1;
    
    sys_write("Enter maximum count (default 10): ");
    if (fgets(input, sizeof(input), stdin) != NULL) {
        int temp = atoi(input);
        if (temp > 0) max_count = temp;
    }
    
    sys_write("Enter delay in seconds (default 1): ");
    if (fgets(input, sizeof(input), stdin) != NULL) {
        int temp = atoi(input);
        if (temp >= 0) delay = temp;
    }
    
    char config[256];
    snprintf(config, sizeof(config), 
             "Starting counter: 1 to %d with %d second delay\n", 
             max_count, delay);
    sys_write(config);
    
    // Main counting loop
    for (int i = 1; i <= max_count; i++) {
        char count_msg[256];
        time_t current_time = time(NULL);
        struct tm* tm_info = localtime(&current_time);
        
        snprintf(count_msg, sizeof(count_msg), 
                "[%02d:%02d:%02d] Count: %d/%d\n",
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                i, max_count);
        sys_write(count_msg);
        
        if (delay > 0 && i < max_count) {
            sleep(delay);
        }
    }
    
    sys_write("Counter program completed successfully!\n");
    sys_exit();
    return 0;
}
