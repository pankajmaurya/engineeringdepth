#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

// Signal handler for segmentation fault
void segfault_handler(int sig) {
    printf("\n🚨 SEGMENTATION FAULT CAUGHT! 🚨\n");
    printf("Signal: %d (SIGSEGV)\n", sig);
    printf("Successfully hit the guard page!\n");
    printf("This proves the guard page protection is working.\n");
    exit(1);
}

void examine_memory_around_stack() {
    int local_var = 0x12345678;  // Give it a recognizable value
    void *stack_addr = &local_var;
    
    printf("=== Stack Guard Page Demonstration ===\n\n");
    printf("Current stack address: %p\n", stack_addr);
    printf("Value of local_var: 0x%x\n", local_var);
    printf("Stack grows downward from here...\n");
    
    // Calculate address 8MB below current stack position
    char *guard_page_area = (char*)stack_addr - (8 * 1024 * 1024);
    printf("Calculated guard page area: %p\n", guard_page_area);
    printf("Distance: %ld bytes (%.2f MB)\n", 
           (char*)stack_addr - guard_page_area,
           ((char*)stack_addr - guard_page_area) / (1024.0 * 1024.0));
    
    printf("\n--- Memory Layout ---\n");
    printf("Stack start (high): %p\n", stack_addr);
    printf("Expected guard:     %p\n", guard_page_area);
    printf("Difference:         %ld bytes\n", (char*)stack_addr - guard_page_area);
    
    printf("\n🎯 Attempting to access guard page area...\n");
    printf("This should trigger a segmentation fault!\n");
    printf("Installing signal handler first...\n");
    
    // Install signal handler to catch the segfault
    signal(SIGSEGV, segfault_handler);
    
    printf("3... 2... 1... 💥\n");
    
    // THE DANGEROUS PART - This should crash!
    // Try to write to the guard page area
    printf("Attempting to write to address: %p\n", guard_page_area);
    *guard_page_area = 'X';  // This line should trigger SIGSEGV
    
    // This line should never execute
    printf("❌ ERROR: We shouldn't reach this line!\n");
    printf("Guard page protection failed!\n");
}

void show_stack_info() {
    // Show current process stack limits
    printf("=== Process Stack Information ===\n");
    
    // Get stack size limit
    struct rlimit rl;
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        printf("Stack limit: %lld bytes (%.2f MB)\n", 
               (long long)rl.rlim_cur, 
               rl.rlim_cur / (1024.0 * 1024.0));
    }
    
    // Show process ID for debugging
    printf("Process ID: %d\n", getpid());
    printf("Page size: %d bytes\n", getpagesize());
    printf("\n");
}

int main() {
    printf("Guard Page Access Demonstration on macOS\n");
    printf("========================================\n\n");
    
    // Show system information first
    show_stack_info();
    
    // Examine stack and attempt guard page access
    examine_memory_around_stack();
    
    // This should never be reached
    printf("Program completed normally (this shouldn't happen!)\n");
    return 0;
}
