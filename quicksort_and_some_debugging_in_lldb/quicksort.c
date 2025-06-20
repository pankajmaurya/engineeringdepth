#include <stdio.h>
#include <stdlib.h>

void swap(int* a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int count, int* arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    int j = low;
    for (; j < high; j++) {
        if (arr[j] > pivot) {
            continue;
        } else {
            ++i;
            swap(arr+i, arr+j);
        }
    }

	// finally swap arr[high] and arr[i+1]
	swap(arr+high, arr+i+1);
    return (i+1);
}

void quicksort(int count, int* arr) {
	if (count == 0) {
		return;
	}
    int pivot_index = partition(count, arr, 0, count - 1);
    quicksort(pivot_index, arr);
    quicksort(count - pivot_index - 1, arr + pivot_index + 1);
}

int main() {
	printf("Enter how many numbers you want to input: \n");
	int count = 0;
	scanf("%d", &count);

	int* arr = (int*) malloc(sizeof(int) * count);
	for (int i = 0; i < count; i++) {
		scanf("%d", arr+i);
	}

	printf("\n Numbers before sorting are: ");
	for (int i = 0; i < count; i++) {
		printf("%d ", arr[i]);
	}

	quicksort(count, arr);
	printf("\n Numbers after sorting are: ");
	for (int i = 0; i < count; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");

	return 0;
}
