#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

// Hardware simulation constants
#define MEMORY_SIZE 65536    // 64KB simulated memory
#define REGISTER_COUNT 8     // 8 general purpose registers
#define STACK_SIZE 1024      // Stack size

// CPU state
typedef struct {
    int registers[REGISTER_COUNT];
    int program_counter;
    int stack_pointer;
    int status_flags;
    int running;
    int interrupt_flag;
} cpu_t;

// Memory system
typedef struct {
    unsigned char memory[MEMORY_SIZE];
    int memory_protection[MEMORY_SIZE/1024]; // Protection bits for 1KB blocks
} memory_system_t;

// I/O system
typedef struct {
    int console_input_ready;
    int console_output_ready;
    char console_buffer[256];
    int disk_busy;
    int timer_interrupt_pending;
} io_system_t;

// Complete hardware state
typedef struct {
    cpu_t cpu;
    memory_system_t memory;
    io_system_t io;
    time_t boot_time;
    int cycle_count;
} hardware_t;

static hardware_t hw;

// Initialize hardware
void hardware_init() {
    memset(&hw, 0, sizeof(hardware_t));
    hw.cpu.stack_pointer = STACK_SIZE - 1;
    hw.cpu.running = 1;
    hw.boot_time = time(NULL);
    
    // Set up memory protection - first 4KB reserved for monitor
    for (int i = 0; i < 4; i++) {
        hw.memory.memory_protection[i] = 1; // Protected
    }
    
    // Initialize I/O system
    hw.io.console_output_ready = 1;
    
    printf("=== HARDWARE SIMULATOR ===\n");
    printf("Memory: %d bytes\n", MEMORY_SIZE);
    printf("Registers: %d\n", REGISTER_COUNT);
    printf("Stack: %d bytes\n", STACK_SIZE);
    printf("Boot time: %s", ctime(&hw.boot_time));
}

// Memory operations with protection
int memory_read(int address, unsigned char* data) {
    if (address < 0 || address >= MEMORY_SIZE) {
        printf("HARDWARE: Memory read fault at address 0x%04X\n", address);
        return -1;
    }
    
    *data = hw.memory.memory[address];
    return 0;
}

int memory_write(int address, unsigned char data) {
    if (address < 0 || address >= MEMORY_SIZE) {
        printf("HARDWARE: Memory write fault at address 0x%04X\n", address);
        return -1;
    }
    
    // Check memory protection
    int block = address / 1024;
    if (hw.memory.memory_protection[block] && hw.cpu.program_counter >= 4096) {
        printf("HARDWARE: Memory protection violation at address 0x%04X\n", address);
        return -1;
    }
    
    hw.memory.memory[address] = data;
    return 0;
}

// Register operations
int register_read(int reg_num) {
    if (reg_num < 0 || reg_num >= REGISTER_COUNT) {
        printf("HARDWARE: Invalid register %d\n", reg_num);
        return 0;
    }
    return hw.cpu.registers[reg_num];
}

void register_write(int reg_num, int value) {
    if (reg_num >= 0 && reg_num < REGISTER_COUNT) {
        hw.cpu.registers[reg_num] = value;
    }
}

// I/O operations
int console_read_char() {
    if (!hw.io.console_input_ready) {
        return -1; // No input available
    }
    
    // Simulate reading from console
    int ch = getchar();
    if (ch == EOF) {
        hw.io.console_input_ready = 0;
        return -1;
    }
    
    printf("HARDWARE: Console input: %c\n", ch);
    return ch;
}

void console_write_char(char ch) {
    if (hw.io.console_output_ready) {
        printf("HARDWARE: Console output: %c", ch);
        fflush(stdout);
    }
}

// Timer interrupt simulation
void timer_interrupt() {
    hw.io.timer_interrupt_pending = 1;
    hw.cpu.interrupt_flag = 1;
}

// Interrupt handling
void handle_interrupts() {
    if (hw.cpu.interrupt_flag) {
        if (hw.io.timer_interrupt_pending) {
            printf("HARDWARE: Timer interrupt\n");
            hw.io.timer_interrupt_pending = 0;
            // Jump to interrupt handler (would be in monitor)
            hw.cpu.program_counter = 0x0100; // Interrupt vector
        }
        hw.cpu.interrupt_flag = 0;
    }
}

// Simple instruction execution simulation
int execute_instruction() {
    unsigned char instruction;
    if (memory_read(hw.cpu.program_counter, &instruction) != 0) {
        return -1;
    }
    
    hw.cpu.program_counter++;
    hw.cycle_count++;
    
    // Simplified instruction set
    switch (instruction) {
        case 0x00: // NOP
            break;
            
        case 0x01: // LOAD immediate to register 0
            {
                unsigned char value;
                memory_read(hw.cpu.program_counter++, &value);
                register_write(0, value);
            }
            break;
            
        case 0x02: // STORE register 0 to memory
            {
                unsigned char addr_low, addr_high;
                memory_read(hw.cpu.program_counter++, &addr_low);
                memory_read(hw.cpu.program_counter++, &addr_high);
                int addr = (addr_high << 8) | addr_low;
                memory_write(addr, register_read(0));
            }
            break;
            
        case 0x03: // ADD register 0 and register 1, store in register 0
            register_write(0, register_read(0) + register_read(1));
            break;
            
        case 0x04: // JUMP
            {
                unsigned char addr_low, addr_high;
                memory_read(hw.cpu.program_counter++, &addr_low);
                memory_read(hw.cpu.program_counter++, &addr_high);
                hw.cpu.program_counter = (addr_high << 8) | addr_low;
            }
            break;
            
        case 0x05: // SYSTEM CALL
            {
                unsigned char syscall_num;
                memory_read(hw.cpu.program_counter++, &syscall_num);
                printf("HARDWARE: System call %d\n", syscall_num);
                // In real system, this would trap to monitor
            }
            break;
            
        case 0xFF: // HALT
            hw.cpu.running = 0;
            printf("HARDWARE: CPU halted\n");
            break;
            
        default:
            printf("HARDWARE: Unknown instruction 0x%02X at PC=0x%04X\n", 
                   instruction, hw.cpu.program_counter - 1);
            return -1;
    }
    
    return 0;
}

// Hardware status display
void hardware_status() {
    printf("\n=== HARDWARE STATUS ===\n");
    printf("CPU Cycles: %d\n", hw.cycle_count);
    printf("Program Counter: 0x%04X\n", hw.cpu.program_counter);
    printf("Stack Pointer: 0x%04X\n", hw.cpu.stack_pointer);
    printf("Status Flags: 0x%02X\n", hw.cpu.status_flags);
    printf("Running: %s\n", hw.cpu.running ? "Yes" : "No");
    
    printf("Registers:\n");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        printf("  R%d: 0x%08X (%d)\n", i, hw.cpu.registers[i], hw.cpu.registers[i]);
    }
    
    printf("I/O Status:\n");
    printf("  Console Input Ready: %s\n", hw.io.console_input_ready ? "Yes" : "No");
    printf("  Console Output Ready: %s\n", hw.io.console_output_ready ? "Yes" : "No");
    printf("  Timer Interrupt Pending: %s\n", hw.io.timer_interrupt_pending ? "Yes" : "No");
    
    printf("Memory Protection:\n");
    for (int i = 0; i < 8; i++) {
        printf("  Block %d (0x%04X-0x%04X): %s\n", 
               i, i * 1024, (i + 1) * 1024 - 1,
               hw.memory.memory_protection[i] ? "Protected" : "User");
    }
    printf("======================\n\n");
}

// Load a simple program into memory
void load_sample_program() {
    printf("Loading sample program...\n");
    
    // Simple program: Load 42 into register 0, then halt
    unsigned char program[] = {
        0x01, 42,    // LOAD immediate 42 into register 0
        0x05, 0x02,  // System call 2 (write)
        0xFF         // HALT
    };
    
    // Load program at address 0x1000 (user space)
    for (int i = 0; i < sizeof(program); i++) {
        memory_write(0x1000 + i, program[i]);
    }
    
    hw.cpu.program_counter = 0x1000;
    printf("Program loaded at 0x1000\n");
}

// Main hardware simulation loop
void hardware_run() {
    printf("\nHardware simulator ready. Commands:\n");
    printf("  RUN - Start CPU execution\n");
    printf("  STEP - Execute one instruction\n");
    printf("  LOAD - Load sample program\n");
    printf("  STATUS - Show hardware status\n");
    printf("  RESET - Reset hardware\n");
    printf("  QUIT - Exit simulator\n");
    
    char command[256];
    
    while (1) {
        printf("HW> ");
        fflush(stdout);
        
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\n")] = 0;
        
        if (strcmp(command, "RUN") == 0) {
            printf("Starting CPU execution...\n");
            while (hw.cpu.running) {
                if (execute_instruction() != 0) {
                    printf("Execution stopped due to error\n");
                    break;
                }
                handle_interrupts();
                
                // Simulate timer interrupts every 1000 cycles
                if (hw.cycle_count % 1000 == 0) {
                    timer_interrupt();
                }
                
                usleep(1000); // Slow down execution for visibility
            }
        }
        else if (strcmp(command, "STEP") == 0) {
            if (hw.cpu.running) {
                execute_instruction();
                handle_interrupts();
            } else {
                printf("CPU is halted\n");
            }
        }
        else if (strcmp(command, "LOAD") == 0) {
            load_sample_program();
        }
        else if (strcmp(command, "STATUS") == 0) {
            hardware_status();
        }
        else if (strcmp(command, "RESET") == 0) {
            hardware_init();
        }
        else if (strcmp(command, "QUIT") == 0) {
            break;
        }
        else if (strlen(command) > 0) {
            printf("Unknown command: %s\n", command);
        }
    }
}

int main() {
    hardware_init();
    hardware_run();
    return 0;
}
