/**
 * Assignment 5: Dynamic Memory Allocation with Pointers
 * Objective: Learn how to allocate and free memory dynamically using pointers.
 * Task:
 * Use malloc() to allocate memory for an integer.
 * Assign a value to the allocated memory and print it.
 * Use malloc() to allocate memory for an array of 5 integers.
 * Populate the array using pointer arithmetic and print the values.
 * Free all allocated memory.
**/

#include <stdlib.h>
#include <stdio.h>

#define ARR_SIZE 5

int main() {
    int *num_p = malloc(sizeof(int));
    if (num_p == NULL) {
        return 1;
    }

    *num_p = 5;

    printf("%d\n", *num_p);

    int *arr_p = malloc(ARR_SIZE * sizeof(int));
    if (arr_p == NULL) {
        return 1;
    }

    for (int i = 0; i < ARR_SIZE; i++) {
        *(arr_p + i) = i;
    }

    for (int i = 0; i < ARR_SIZE; i++) {
        printf("%d\n", *(arr_p + i));
    }

    free(num_p);
    free(arr_p);

    return 0;
}
