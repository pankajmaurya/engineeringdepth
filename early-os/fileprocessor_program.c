#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void sys_write(const char* message) {
    printf("FILE PROCESSOR: %s", message);
    fflush(stdout);
}

void sys_exit() {
    printf("FILE PROCESSOR: Requesting program termination\n");
    exit(0);
}

// Simulate file processing work
void process_files() {
    const char* files[] = {
        "document1.txt",
        "report.pdf", 
        "data.csv",
        "config.xml",
        "backup.zip"
    };
    int num_files = sizeof(files) / sizeof(files[0]);
    
    sys_write("Beginning file processing simulation...\n");
    
    for (int i = 0; i < num_files; i++) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Processing file %d/%d: %s\n", 
                i + 1, num_files, files[i]);
        sys_write(msg);
        
        // Simulate different processing times
        int process_time = 1 + (i % 3); // 1-3 seconds
        
        for (int j = 0; j < process_time; j++) {
            char progress[256];
            snprintf(progress, sizeof(progress), 
                    "  [%s] Processing... %d/%d seconds\n", 
                    files[i], j + 1, process_time);
            sys_write(progress);
            sleep(1);
        }
        
        snprintf(msg, sizeof(msg), "  [%s] Processing complete!\n", files[i]);
        sys_write(msg);
    }
    
    sys_write("All files processed successfully!\n");
}

int main(int argc, char* argv[]) {
    int program_id = 0;
    
    if (argc > 1) {
        program_id = atoi(argv[1]);
    }
    
    char welcome[256];
    snprintf(welcome, sizeof(welcome), 
             "File processor started (ID: %d)\n", program_id);
    sys_write(welcome);
    
    // Show start time
    time_t start_time = time(NULL);
    char time_msg[256];
    snprintf(time_msg, sizeof(time_msg), "Start time: %s", ctime(&start_time));
    sys_write(time_msg);
    
    // Do the work
    process_files();
    
    // Show completion time and duration
    time_t end_time = time(NULL);
    snprintf(time_msg, sizeof(time_msg), "End time: %s", ctime(&end_time));
    sys_write(time_msg);
    
    snprintf(time_msg, sizeof(time_msg), "Total duration: %.0f seconds\n", 
             difftime(end_time, start_time));
    sys_write(time_msg);
    
    sys_exit();
    return 0;
}
