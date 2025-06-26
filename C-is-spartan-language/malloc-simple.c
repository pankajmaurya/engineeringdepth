#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>

// Minimum allocation size (16 bytes for alignment)
#define MIN_CHUNK_SIZE 16
#define ALIGNMENT 8
#define ALIGN(x) (((x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

// Chunk header flags
#define PREV_INUSE 0x1
#define IS_MMAPPED 0x2
#define NON_MAIN_ARENA 0x4

// Size threshold for mmap (128KB)
#define MMAP_THRESHOLD (128 * 1024)

// Chunk structure
typedef struct chunk {
    size_t prev_size;    // Size of previous chunk (if free)
    size_t size;         // Size of this chunk + flags in low 3 bits
    struct chunk *fd;    // Forward pointer (free chunks only)
    struct chunk *bk;    // Backward pointer (free chunks only)
} chunk_t;

// Get size without flags
#define chunk_size(p) ((p)->size & ~(PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA))

// Check if chunk is in use
#define chunk_inuse(p) ((p)->size & PREV_INUSE)

// Get next chunk
#define next_chunk(p) ((chunk_t*)((char*)(p) + chunk_size(p)))

// Get previous chunk (only if previous is free)
#define prev_chunk(p) ((chunk_t*)((char*)(p) - (p)->prev_size))

// Convert chunk to user pointer
#define chunk_to_mem(p) ((void*)((char*)(p) + 2 * sizeof(size_t)))

// Convert user pointer to chunk
#define mem_to_chunk(mem) ((chunk_t*)((char*)(mem) - 2 * sizeof(size_t)))

// Simple free list (unsorted bin)
static chunk_t *free_list = NULL;

// Top chunk (wilderness)
static chunk_t *top_chunk = NULL;

// Heap start
static void *heap_start = NULL;

// Initialize heap with sbrk
static int init_heap() {
    if (heap_start) return 0;
    
    // Get initial heap space (64KB)
    size_t initial_size = 64 * 1024;
    heap_start = sbrk(initial_size);
    
    if (heap_start == (void*)-1) {
        heap_start = NULL;
        return -1;
    }
    
    // Initialize top chunk
    top_chunk = (chunk_t*)heap_start;
    top_chunk->prev_size = 0;
    top_chunk->size = initial_size - 2 * sizeof(size_t);
    top_chunk->fd = NULL;
    top_chunk->bk = NULL;
    
    return 0;
}

// Add chunk to free list
static void add_to_free_list(chunk_t *chunk) {
    chunk->fd = free_list;
    chunk->bk = NULL;
    
    if (free_list) {
        free_list->bk = chunk;
    }
    
    free_list = chunk;
}

// Remove chunk from free list
static void remove_from_free_list(chunk_t *chunk) {
    if (chunk->fd) {
        chunk->fd->bk = chunk->bk;
    }
    
    if (chunk->bk) {
        chunk->bk->fd = chunk->fd;
    } else {
        free_list = chunk->fd;
    }
}

// Find suitable free chunk (first fit)
static chunk_t* find_free_chunk(size_t size) {
    chunk_t *current = free_list;
    
    while (current) {
        if (chunk_size(current) >= size) {
            return current;
        }
        current = current->fd;
    }
    
    return NULL;
}

// Split chunk if it's too large
static void split_chunk(chunk_t *chunk, size_t size) {
    size_t old_size = chunk_size(chunk);
    
    if (old_size >= size + MIN_CHUNK_SIZE) {
        // Create new chunk from remainder
        chunk_t *new_chunk = (chunk_t*)((char*)chunk + size);
        new_chunk->prev_size = size;
        new_chunk->size = old_size - size;
        new_chunk->fd = NULL;
        new_chunk->bk = NULL;
        
        // Update current chunk size
        chunk->size = size | (chunk->size & (PREV_INUSE | IS_MMAPPED | NON_MAIN_ARENA));
        
        // Add remainder to free list
        add_to_free_list(new_chunk);
    }
}

// Coalesce adjacent free chunks
static chunk_t* coalesce(chunk_t *chunk) {
    size_t size = chunk_size(chunk);
    chunk_t *next = next_chunk(chunk);
    
    // Coalesce with next chunk if free
    if ((char*)next < (char*)heap_start + 64 * 1024 && 
        next != top_chunk && !chunk_inuse(next)) {
        remove_from_free_list(next);
        size += chunk_size(next);
        chunk->size = size;
    }
    
    // Coalesce with previous chunk if free
    if (!(chunk->size & PREV_INUSE)) {
        chunk_t *prev = prev_chunk(chunk);
        remove_from_free_list(prev);
        size += chunk_size(prev);
        prev->size = size;
        chunk = prev;
    }
    
    return chunk;
}

// Expand heap using sbrk
static int expand_heap(size_t size) {
    void *prev_brk = sbrk(size);
    if (prev_brk == (void*)-1) {
        return -1;
    }
    
    // Expand top chunk
    top_chunk->size += size;
    return 0;
}

// Main malloc implementation
void* my_malloc(size_t size) {
    if (size == 0) return NULL;
    
    // Initialize heap if needed
    if (!heap_start && init_heap() < 0) {
        return NULL;
    }
    
    // Align size and add header overhead
    size_t aligned_size = ALIGN(size + 2 * sizeof(size_t));
    if (aligned_size < MIN_CHUNK_SIZE) {
        aligned_size = MIN_CHUNK_SIZE;
    }
    
    // Use mmap for large allocations
    if (size >= MMAP_THRESHOLD) {
        size_t mmap_size = ALIGN(size + sizeof(chunk_t));
        void *mem = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (mem == MAP_FAILED) {
            return NULL;
        }
        
        chunk_t *chunk = (chunk_t*)mem;
        chunk->prev_size = 0;
        chunk->size = mmap_size | IS_MMAPPED;
        
        return chunk_to_mem(chunk);
    }
    
    // Try to find suitable free chunk
    chunk_t *chunk = find_free_chunk(aligned_size);
    
    if (chunk) {
        // Remove from free list and split if necessary
        remove_from_free_list(chunk);
        split_chunk(chunk, aligned_size);
        chunk->size |= PREV_INUSE;
        return chunk_to_mem(chunk);
    }
    
    // Use top chunk
    if (chunk_size(top_chunk) >= aligned_size) {
        chunk = top_chunk;
        
        if (chunk_size(top_chunk) > aligned_size) {
            // Split top chunk
            top_chunk = (chunk_t*)((char*)top_chunk + aligned_size);
            top_chunk->prev_size = aligned_size;
            top_chunk->size = chunk_size(chunk) - aligned_size;
        } else {
            // Used entire top chunk
            top_chunk = NULL;
        }
        
        chunk->size = aligned_size | PREV_INUSE;
        return chunk_to_mem(chunk);
    }
    
    // Expand heap
    size_t expand_size = aligned_size > 4096 ? aligned_size : 4096;
    if (expand_heap(expand_size) < 0) {
        return NULL;
    }
    
    // Retry with expanded heap
    return my_malloc(size);
}

// Main free implementation
void my_free(void* ptr) {
    if (!ptr) return;
    
    chunk_t *chunk = mem_to_chunk(ptr);
    
    // Handle mmap'd chunks
    if (chunk->size & IS_MMAPPED) {
        size_t size = chunk_size(chunk);
        munmap(chunk, size);
        return;
    }
    
    // Clear in-use flag
    chunk->size &= ~PREV_INUSE;
    
    // Coalesce with adjacent free chunks
    chunk = coalesce(chunk);
    
    // Add to free list
    add_to_free_list(chunk);
}

// Realloc implementation
void* my_realloc(void* ptr, size_t size) {
    if (!ptr) return my_malloc(size);
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }
    
    chunk_t *chunk = mem_to_chunk(ptr);
    size_t old_size = chunk_size(chunk) - 2 * sizeof(size_t);
    
    if (size <= old_size) {
        return ptr; // Current chunk is large enough
    }
    
    // Allocate new chunk and copy data
    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, old_size);
    my_free(ptr);
    
    return new_ptr;
}

// Test function
void test_malloc() {
    printf("Testing minimal ptmalloc implementation...\n");
    
    // Test basic allocation
    void *p1 = my_malloc(100);
    printf("Allocated 100 bytes at %p\n", p1);
    
    void *p2 = my_malloc(200);
    printf("Allocated 200 bytes at %p\n", p2);
    
    // Test free
    my_free(p1);
    printf("Freed first allocation\n");
    
    // Test reallocation
    void *p3 = my_malloc(50);
    printf("Allocated 50 bytes at %p (should reuse freed space)\n", p3);
    
    // Test large allocation (should use mmap)
    void *p4 = my_malloc(200 * 1024);
    printf("Allocated 200KB at %p (using mmap)\n", p4);
    
    // Test realloc
    p3 = my_realloc(p3, 150);
    printf("Reallocated to 150 bytes at %p\n", p3);
    
    // Cleanup
    my_free(p2);
    my_free(p3);
    my_free(p4);
    
    printf("All tests completed successfully!\n");
}

int main() {
    test_malloc();
    return 0;
}
