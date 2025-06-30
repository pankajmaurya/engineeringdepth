#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 100
#define NUM_ITEMS 1000

typedef int Item;

typedef struct {
    Item buffer[N];
    int in, out, size;
    pthread_mutex_t mutex;
    pthread_cond_t not_full, not_empty;
} BoundedBuffer;

BoundedBuffer buf;

void BoundedBuffer_init(BoundedBuffer *b) {
    b->in = b->out = b->size = 0;
    
    // Initialize mutex with default attributes (non-recursive)
    pthread_mutex_init(&b->mutex, NULL);  // NULL means use default mutex attributes
    
    // Initialize condition variable for 'not full' with default attributes
    pthread_cond_init(&b->not_full, NULL);  // NULL means use default cond var attributes

    // Initialize condition variable for 'not empty' with default attributes
    pthread_cond_init(&b->not_empty, NULL);  // NULL means use default cond var attributes
}

void BoundedBuffer_add(BoundedBuffer *b, Item i) {
    pthread_mutex_lock(&b->mutex);
    while (b->size == N)
        pthread_cond_wait(&b->not_full, &b->mutex);  // Wait until buffer is not full
    b->buffer[b->in] = i;
    b->in = (b->in + 1) % N;
    b->size++;
    pthread_cond_signal(&b->not_empty);  // Signal that buffer is no longer empty
    pthread_mutex_unlock(&b->mutex);
}

Item BoundedBuffer_consume(BoundedBuffer *b) {
    pthread_mutex_lock(&b->mutex);
    while (b->size == 0)
        pthread_cond_wait(&b->not_empty, &b->mutex);  // Wait until buffer is not empty
    Item i = b->buffer[b->out];
    b->out = (b->out + 1) % N;
    b->size--;
    pthread_cond_signal(&b->not_full);  // Signal that buffer is no longer full
    pthread_mutex_unlock(&b->mutex);
    return i;
}

void* producer(void* arg) {
    for (int i = 0; i < NUM_ITEMS; ++i) {
        BoundedBuffer_add(&buf, i);
        printf("Produced: %d\n", i);
        usleep(1000);
    }
    return NULL;  // Return NULL since thread doesn't return anything
}

void* consumer(void* arg) {
    for (int i = 0; i < NUM_ITEMS; ++i) {
        Item item = BoundedBuffer_consume(&buf);
        printf("Consumed: %d\n", item);
        usleep(2000);
    }
    return NULL;  // Return NULL since thread doesn't return anything
}

int main() {
    pthread_t prod, cons;

    BoundedBuffer_init(&buf);

    // Create producer thread with default attributes
    pthread_create(&prod, NULL, producer, NULL);  // 1st NULL: default thread attr, 2nd NULL: no args

    // Create consumer thread with default attributes
    pthread_create(&cons, NULL, consumer, NULL);  // same as above

    pthread_join(prod, NULL);  // Wait for producer to finish; no return value
    pthread_join(cons, NULL);  // Wait for consumer to finish; no return value

    return 0;
}
