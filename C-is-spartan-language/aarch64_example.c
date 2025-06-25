#include <stdio.h>
#include <stdint.h>

// Function using inline assembly to add two numbers
int64_t add_with_assembly(int64_t a, int64_t b) {
    int64_t result;
    
    // AArch64 inline assembly
    __asm__ volatile (
        "add %0, %1, %2"        // Add x1 and x2, store in x0
        : "=r" (result)         // Output: result in a general-purpose register
        : "r" (a), "r" (b)      // Inputs: a and b in general-purpose registers
        :                       // No clobbered registers
    );
    
    return result;
}

// Function using basic arithmetic operations
int64_t multiply_with_assembly(int64_t a, int64_t b) {
    int64_t result;
    
    __asm__ volatile (
        "mul %0, %1, %2"        // Multiply x1 and x2, store in x0
        : "=r" (result)
        : "r" (a), "r" (b)
        :
    );
    
    return result;
}

// Function to demonstrate bit manipulation
uint64_t count_leading_zeros(uint64_t value) {
    uint64_t result;
    
    __asm__ volatile (
        "clz %0, %1"            // Count leading zeros
        : "=r" (result)
        : "r" (value)
        :
    );
    
    return result;
}

// Function to demonstrate conditional operations
int64_t conditional_increment(int64_t value, int64_t threshold) {
    int64_t result;
    
    __asm__ volatile (
        "cmp %1, %2         \n\t"   // Compare value with threshold
        "cinc %0, %1, gt    \n\t"   // Conditional increment if greater
        : "=r" (result)
        : "r" (value), "r" (threshold)
        : "cc"                      // Condition codes are modified
    );
    
    return result;
}

// Simple NEON example - safer version
void simple_vector_add() {
    // Use simpler approach with basic NEON
    float vec1[2] = {1.5f, 2.5f};
    float vec2[2] = {3.5f, 4.5f};
    float result[2];
    
    __asm__ volatile (
        "ldr d0, [%1]           \n\t"  // Load vec1 into d0 (64-bit)
        "ldr d1, [%2]           \n\t"  // Load vec2 into d1 (64-bit)
        "fadd v2.2s, v0.2s, v1.2s \n\t" // Vector float add
        "str d2, [%0]           \n\t"  // Store result
        :                               // No outputs
        : "r" (result), "r" (vec1), "r" (vec2)
        : "v0", "v1", "v2", "memory"
    );
    
    printf("NEON Vector Addition (floats):\n");
    printf("  %.1f + %.1f = %.1f\n", vec1[0], vec2[0], result[0]);
    printf("  %.1f + %.1f = %.1f\n", vec1[1], vec2[1], result[1]);
}

// Function to read a user-accessible timer
uint64_t get_virtual_counter() {
    uint64_t count;
    
    __asm__ volatile (
        "mrs %0, cntvct_el0"    // Read virtual counter
        : "=r" (count)
        :
        :
    );
    
    return count;
}

int main() {
    printf("AArch64 Assembly Examples on Apple Silicon\n");
    printf("==========================================\n\n");
    
    // Basic arithmetic with assembly
    int64_t num1 = 42, num2 = 13;
    int64_t sum = add_with_assembly(num1, num2);
    printf("Assembly Addition: %lld + %lld = %lld\n", num1, num2, sum);
    
    int64_t product = multiply_with_assembly(num1, num2);
    printf("Assembly Multiplication: %lld * %lld = %lld\n\n", num1, num2, product);
    
    // Bit manipulation
    uint64_t test_value = 0x0000FFFF00000000ULL;
    uint64_t leading_zeros = count_leading_zeros(test_value);
    printf("Bit Operations:\n");
    printf("  Value: 0x%016llx\n", test_value);
    printf("  Leading zeros: %llu\n\n", leading_zeros);
    
    // Conditional operations
    int64_t val = 10, threshold = 5;
    int64_t cond_result = conditional_increment(val, threshold);
    printf("Conditional Operations:\n");
    printf("  Value: %lld, Threshold: %lld\n", val, threshold);
    printf("  Result (increment if > threshold): %lld\n\n", cond_result);
    
    // NEON SIMD example
    simple_vector_add();
    printf("\n");
    
    // Timer access
    uint64_t timer1 = get_virtual_counter();
    // Do some work
    volatile int dummy = 0;
    for (int i = 0; i < 1000; i++) {
        dummy += i;
    }
    uint64_t timer2 = get_virtual_counter();
    
    printf("Timer Operations:\n");
    printf("  Start counter: %llu\n", timer1);
    printf("  End counter: %llu\n", timer2);
    printf("  Elapsed ticks: %llu\n\n", timer2 - timer1);
    
    // Architecture-specific information
    printf("Architecture Notes:\n");
    printf("  - Compiled for AArch64 (ARM64)\n");
    printf("  - Compatible with Apple M1/M2/M3/M4 chips\n");
    printf("  - Uses 64-bit registers (x0-x30)\n");
    printf("  - NEON SIMD instructions available\n");
    printf("  - User-space accessible timers\n");
    printf("  - Conditional instructions (CINC)\n");
    
    return 0;
}

/*
Compilation instructions for macOS:
    clang -arch arm64 -O2 -o aarch64_example aarch64_example.c

Key AArch64 features demonstrated:
1. Basic arithmetic instructions (ADD, MUL)
2. Bit manipulation (CLZ - Count Leading Zeros)
3. Conditional instructions (CMP, CINC)
4. NEON SIMD vector operations (FADD.2S)
5. User-accessible system timer (CNTVCT_EL0)

This version avoids:
- Privileged system registers (EL1 level)
- Complex atomic operations that might not work in user space
- Instructions that might be restricted on macOS

Register conventions:
- x0-x30: General-purpose 64-bit registers
- w0-w30: 32-bit views of x registers
- v0-v31: NEON/floating-point registers (d0-d31 for 64-bit views)
- sp: Stack pointer
- lr (x30): Link register
*/
