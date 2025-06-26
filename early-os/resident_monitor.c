#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

// Monitor system constants
#define MAX_PROGRAMS 16
#define MAX_MEMORY_BLOCKS 32
#define MEMORY_BLOCK_SIZE 1024
#define MAX_FILENAME 256

// System call codes (historical style)
#define SYS_READ    1
#define SYS_WRITE   2
#define SYS_LOAD    3
#define SYS_EXIT    4
#define SYS_TIME    5
#define SYS_ALLOC   6
#define SYS_FREE    7

// Memory management structure
typedef struct {
    int allocated;
    int program_id;
    void* address;
} memory_block_t;

// Program control block
typedef struct {
    int program_id;
    char filename[MAX_FILENAME];
    pid_t pid;
    int status;
    time_t load_time;
} program_t;

// Monitor state
typedef struct {
    memory_block_t memory[MAX_MEMORY_BLOCKS];
    program_t programs[MAX_PROGRAMS];
    int next_program_id;
    int total_programs_run;
    time_t boot_time;
} monitor_t;

static monitor_t monitor;

// Initialize the monitor
void monitor_init() {
    memset(&monitor, 0, sizeof(monitor_t));
    monitor.next_program_id = 1;
    monitor.boot_time = time(NULL);
    
    // Initialize memory blocks
    for (int i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        monitor.memory[i].allocated = 0;
        monitor.memory[i].program_id = 0;
        monitor.memory[i].address = malloc(MEMORY_BLOCK_SIZE);
    }
    
    printf("=== RESIDENT MONITOR v1.0 ===\n");
    printf("Monitor initialized with %d memory blocks of %d bytes each\n", 
           MAX_MEMORY_BLOCKS, MEMORY_BLOCK_SIZE);
    printf("Boot time: %s", ctime(&monitor.boot_time));
}

// Memory allocation service
int monitor_alloc_memory(int program_id) {
    for (int i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        if (!monitor.memory[i].allocated) {
            monitor.memory[i].allocated = 1;
            monitor.memory[i].program_id = program_id;
            return i; // Return block ID
        }
    }
    return -1; // No memory available
}

// Memory deallocation service
int monitor_free_memory(int program_id, int block_id) {
    if (block_id >= 0 && block_id < MAX_MEMORY_BLOCKS) {
        if (monitor.memory[block_id].allocated && 
            monitor.memory[block_id].program_id == program_id) {
            monitor.memory[block_id].allocated = 0;
            monitor.memory[block_id].program_id = 0;
            return 0; // Success
        }
    }
    return -1; // Error
}

// Program loading service
int monitor_load_program(const char* filename) {
    // Find available program slot
    int slot = -1;
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        if (monitor.programs[i].program_id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("ERROR: No program slots available\n");
        return -1;
    }
    
    // Allocate memory for the program
    int memory_block = monitor_alloc_memory(monitor.next_program_id);
    if (memory_block == -1) {
        printf("ERROR: Insufficient memory to load program\n");
        return -1;
    }
    
    // Set up program control block
    program_t* prog = &monitor.programs[slot];
    prog->program_id = monitor.next_program_id++;
    strncpy(prog->filename, filename, MAX_FILENAME - 1);
    prog->load_time = time(NULL);
    
    printf("MONITOR: Loading program %s (ID: %d, Memory block: %d)\n", 
           filename, prog->program_id, memory_block);
    
    // For simulation, we'll just fork and exec the program
    prog->pid = fork();
    if (prog->pid == 0) {
        // Child process - execute the user program
        char prog_id_str[16];
        snprintf(prog_id_str, sizeof(prog_id_str), "%d", prog->program_id);
        execl(filename, filename, prog_id_str, NULL);
        exit(1); // If exec fails
    } else if (prog->pid > 0) {
        // Parent process - monitor continues
        monitor.total_programs_run++;
        return prog->program_id;
    } else {
        // Fork failed
        monitor_free_memory(prog->program_id, memory_block);
        prog->program_id = 0;
        return -1;
    }
}

// I/O services
void monitor_console_write(const char* message) {
    printf("PROGRAM OUTPUT: %s", message);
    fflush(stdout);
}

int monitor_console_read(char* buffer, int max_len) {
    printf("PROGRAM INPUT: ");
    fflush(stdout);
    if (fgets(buffer, max_len, stdin) != NULL) {
        return strlen(buffer);
    }
    return 0;
}

// System call handler (simplified simulation)
int monitor_system_call(int call_code, int program_id, void* param1, void* param2) {
    switch (call_code) {
        case SYS_READ:
            return monitor_console_read((char*)param1, (int)(long)param2);
            
        case SYS_WRITE:
            monitor_console_write((char*)param1);
            return 0;
            
        case SYS_LOAD:
            return monitor_load_program((char*)param1);
            
        case SYS_TIME: {
            time_t current_time = time(NULL);
            *(time_t*)param1 = current_time;
            return 0;
        }
        
        case SYS_ALLOC:
            return monitor_alloc_memory(program_id);
            
        case SYS_FREE:
            return monitor_free_memory(program_id, (int)(long)param1);
            
        case SYS_EXIT:
            // Clean up program resources
            for (int i = 0; i < MAX_MEMORY_BLOCKS; i++) {
                if (monitor.memory[i].program_id == program_id) {
                    monitor.memory[i].allocated = 0;
                    monitor.memory[i].program_id = 0;
                }
            }
            return 0;
            
        default:
            printf("ERROR: Unknown system call %d\n", call_code);
            return -1;
    }
}

// Monitor status display
void monitor_status() {
    printf("\n=== MONITOR STATUS ===\n");
    printf("Uptime: %.0f seconds\n", difftime(time(NULL), monitor.boot_time));
    printf("Total programs run: %d\n", monitor.total_programs_run);
    printf("Next program ID: %d\n", monitor.next_program_id);
    
    // Memory usage
    int allocated_blocks = 0;
    for (int i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        if (monitor.memory[i].allocated) allocated_blocks++;
    }
    printf("Memory: %d/%d blocks allocated\n", allocated_blocks, MAX_MEMORY_BLOCKS);
    
    // Active programs
    printf("Active programs:\n");
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        if (monitor.programs[i].program_id != 0) {
            printf("  ID %d: %s (PID: %d)\n", 
                   monitor.programs[i].program_id,
                   monitor.programs[i].filename,
                   monitor.programs[i].pid);
        }
    }
    printf("=====================\n\n");
}

// Main monitor loop
void monitor_loop() {
    char command[256];
    char filename[MAX_FILENAME];
    
    printf("\nMonitor ready. Available commands:\n");
    printf("  LOAD <filename> - Load and execute a program\n");
    printf("  STATUS - Show monitor status\n");
    printf("  HALT - Shutdown monitor\n");
    
    while (1) {
        printf("MONITOR> ");
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        // Remove newline
        command[strcspn(command, "\n")] = 0;
        
        if (strncmp(command, "LOAD ", 5) == 0) {
            sscanf(command + 5, "%s", filename);
            int program_id = monitor_load_program(filename);
            if (program_id > 0) {
                printf("Program loaded with ID: %d\n", program_id);
                
                // Wait for program completion
                int status;
                for (int i = 0; i < MAX_PROGRAMS; i++) {
                    if (monitor.programs[i].program_id == program_id) {
                        waitpid(monitor.programs[i].pid, &status, 0);
                        printf("Program %d terminated with status %d\n", program_id, status);
                        
                        // Clean up program slot
                        monitor.programs[i].program_id = 0;
                        break;
                    }
                }
            }
        }
        else if (strcmp(command, "STATUS") == 0) {
            monitor_status();
        }
        else if (strcmp(command, "HALT") == 0) {
            printf("Shutting down monitor...\n");
            break;
        }
        else if (strlen(command) > 0) {
            printf("Unknown command: %s\n", command);
        }
    }
}

// Cleanup
void monitor_cleanup() {
    for (int i = 0; i < MAX_MEMORY_BLOCKS; i++) {
        if (monitor.memory[i].address) {
            free(monitor.memory[i].address);
        }
    }
}

int main() {
    monitor_init();
    monitor_loop();
    monitor_cleanup();
    return 0;
}
