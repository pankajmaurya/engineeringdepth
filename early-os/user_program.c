#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// System call interface (matching monitor definitions)
#define SYS_READ    1
#define SYS_WRITE   2
#define SYS_LOAD    3
#define SYS_EXIT    4
#define SYS_TIME    5
#define SYS_ALLOC   6
#define SYS_FREE    7

// Simulate system calls by printing what would happen
// In a real system, these would trap to the monitor
void sys_write(const char* message) {
    printf("USER PROGRAM: %s", message);
    fflush(stdout);
}

int sys_read(char* buffer, int max_len) {
    printf("USER PROGRAM requesting input: ");
    fflush(stdout);
    if (fgets(buffer, max_len, stdin) != NULL) {
        return strlen(buffer);
    }
    return 0;
}

void sys_time(time_t* t) {
    *t = time(NULL);
}

int sys_alloc() {
    // Simulate memory allocation request
    printf("USER PROGRAM: Requesting memory allocation\n");
    return 0; // Simulate successful allocation
}

void sys_free(int block_id) {
    printf("USER PROGRAM: Freeing memory block %d\n", block_id);
}

void sys_exit() {
    printf("USER PROGRAM: Requesting program termination\n");
    exit(0);
}

// Simple calculator program
void calculator() {
    char input[256];
    float a, b, result;
    char op;
    
    sys_write("=== Simple Calculator ===\n");
    sys_write("Enter calculations in format: number operator number\n");
    sys_write("Operators: +, -, *, /\n");
    sys_write("Enter 'quit' to exit\n\n");
    
    while (1) {
        sys_write("Calculator> ");
        
        if (sys_read(input, sizeof(input)) == 0) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "quit") == 0) {
            break;
        }
        
        if (sscanf(input, "%f %c %f", &a, &op, &b) == 3) {
            switch (op) {
                case '+':
                    result = a + b;
                    break;
                case '-':
                    result = a - b;
                    break;
                case '*':
                    result = a * b;
                    break;
                case '/':
                    if (b != 0) {
                        result = a / b;
                    } else {
                        sys_write("Error: Division by zero\n");
                        continue;
                    }
                    break;
                default:
                    sys_write("Error: Unknown operator\n");
                    continue;
            }
            
            char output[256];
            snprintf(output, sizeof(output), "Result: %.2f\n", result);
            sys_write(output);
        } else {
            sys_write("Error: Invalid input format\n");
        }
    }
}

// Memory test program
void memory_test() {
    sys_write("=== Memory Test Program ===\n");
    
    // Request memory allocation
    int block1 = sys_alloc();
    char msg[256];
    snprintf(msg, sizeof(msg), "Allocated memory block: %d\n", block1);
    sys_write(msg);
    
    // Request another block
    int block2 = sys_alloc();
    snprintf(msg, sizeof(msg), "Allocated memory block: %d\n", block2);
    sys_write(msg);
    
    // Simulate using the memory
    sys_write("Simulating memory usage...\n");
    sleep(2);
    
    // Free the memory
    sys_free(block1);
    sys_free(block2);
    
    sys_write("Memory test completed\n");
}

// System information program
void system_info() {
    time_t current_time;
    sys_time(&current_time);
    
    sys_write("=== System Information ===\n");
    
    char info[512];
    snprintf(info, sizeof(info), "Current time: %s", ctime(&current_time));
    sys_write(info);
    
    snprintf(info, sizeof(info), "Program ID: %s\n", 
             getenv("PROGRAM_ID") ? getenv("PROGRAM_ID") : "Unknown");
    sys_write(info);
    
    snprintf(info, sizeof(info), "Process ID: %d\n", getpid());
    sys_write(info);
    
    sys_write("System information displayed\n");
}

int main(int argc, char* argv[]) {
    int program_id = 0;
    
    // Get program ID from command line (passed by monitor)
    if (argc > 1) {
        program_id = atoi(argv[1]);
    }
    
    char welcome[256];
    snprintf(welcome, sizeof(welcome), 
             "User program started (ID: %d)\n", program_id);
    sys_write(welcome);
    
    // Simple menu system
    char choice[256];
    
    while (1) {
        sys_write("\n=== USER PROGRAM MENU ===\n");
        sys_write("1. Calculator\n");
        sys_write("2. Memory Test\n");
        sys_write("3. System Info\n");
        sys_write("4. Exit\n");
        sys_write("Choose option (1-4): ");
        
        if (sys_read(choice, sizeof(choice)) == 0) {
            break;
        }
        
        switch (choice[0]) {
            case '1':
                calculator();
                break;
            case '2':
                memory_test();
                break;
            case '3':
                system_info();
                break;
            case '4':
                sys_write("Exiting program...\n");
                sys_exit();
                break;
            default:
                sys_write("Invalid choice. Please try again.\n");
                break;
        }
    }
    
    sys_exit();
    return 0;
}
