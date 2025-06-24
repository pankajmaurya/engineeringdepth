#include <stdio.h>

void deep_function(int depth) {
    char buffer[1024];  // 1KB local array
    printf("Depth %d, buffer at %p\n", depth, buffer);
    
    if (depth > 0) {
        deep_function(depth - 1);  // Recursive call
    }
}

int main() {
    printf("Main thread stack limit: ~1MB\n");
    deep_function(8000);  // This will likely cause stack overflow
    return 0;
}
