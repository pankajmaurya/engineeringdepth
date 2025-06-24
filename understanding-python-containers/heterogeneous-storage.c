#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define types for our objects
typedef enum {
    TYPE_INT,
    TYPE_STRING,
    TYPE_DOUBLE
} ObjectType;

// Base object structure (like PyObject)
typedef struct {
    ObjectType type;
    void *data;
} Object;

// Our dynamic list structure
typedef struct {
    Object **items;    // Array of pointers to objects
    int size;          // Current number of items
    int capacity;      // Allocated capacity
} List;

// Create a new object
Object* create_object(ObjectType type, void *value) {
    Object *obj = malloc(sizeof(Object));
    obj->type = type;
    
    switch (type) {
        case TYPE_INT:
            obj->data = malloc(sizeof(int));
            *(int*)obj->data = *(int*)value;
            break;
        case TYPE_STRING:
            obj->data = malloc(strlen((char*)value) + 1);
            strcpy((char*)obj->data, (char*)value);
            break;
        case TYPE_DOUBLE:
            obj->data = malloc(sizeof(double));
            *(double*)obj->data = *(double*)value;
            break;
    }
    return obj;
}

// Create a new list
List* create_list() {
    List *list = malloc(sizeof(List));
    list->items = malloc(sizeof(Object*) * 2);
    list->size = 0;
    list->capacity = 2;
    return list;
}

// Add item to list (like Python's append)
void list_append(List *list, Object *obj) {
    if (list->size >= list->capacity) {
        // Grow the array (double capacity)
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(Object*) * list->capacity);
    }
    list->items[list->size++] = obj;
}

// Print an object based on its type
void print_object(Object *obj) {
    switch (obj->type) {
        case TYPE_INT:
            printf("%d", *(int*)obj->data);
            break;
        case TYPE_STRING:
            printf("\"%s\"", (char*)obj->data);
            break;
        case TYPE_DOUBLE:
            printf("%.2f", *(double*)obj->data);
            break;
    }
}

// Print entire list
void print_list(List *list) {
    printf("[");
    for (int i = 0; i < list->size; i++) {
        print_object(list->items[i]);
        if (i < list->size - 1) printf(", ");
    }
    printf("]\n");
}

// Free memory
void free_object(Object *obj) {
    free(obj->data);
    free(obj);
}

void free_list(List *list) {
    for (int i = 0; i < list->size; i++) {
        free_object(list->items[i]);
    }
    free(list->items);
    free(list);
}

int main() {
    // Create a heterogeneous list
    List *mylist = create_list();
    
    // Add different types of data
    int num = 42;
    char *text = "hello";
    double pi = 3.14;
    
    list_append(mylist, create_object(TYPE_INT, &num));
    list_append(mylist, create_object(TYPE_STRING, text));
    list_append(mylist, create_object(TYPE_DOUBLE, &pi));
    
    // Add more items to trigger reallocation
    int another_num = 100;
    list_append(mylist, create_object(TYPE_INT, &another_num));
    
    // Print the list
    printf("List contents: ");
    print_list(mylist);
    printf("List size: %d, capacity: %d\n", mylist->size, mylist->capacity);
    
    // Clean up
    free_list(mylist);
    return 0;
}
