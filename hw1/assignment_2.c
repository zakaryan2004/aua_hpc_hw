/**
 * Assignment 2: Pointer Arithmetic
 * Objective: Learn how pointer arithmetic works.
 * Task:
 * Declare an array of integers and initialize it with 5 values.
 * Use a pointer to traverse the array and print each element.
 * Modify the values of the array using pointer arithmetic.
 * Print the modified array using both the pointer and the array name.
**/

#include <stdio.h>

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    int arrlen = sizeof(arr) / sizeof(arr[0]);

    int *arr_p = arr;
    for (int i = 0; i < arrlen; i++) {
        printf("%d\n", *arr_p);
        arr_p++;
    }
    printf("\n\n");

    for (int i = 0; i < arrlen; i++) {
        *(arr + i) += 1;
    }
    printf("\n\n");

    arr_p = arr;
    for (int i = 0; i < arrlen; i++) {
        printf("%d\n", *arr_p);
        arr_p++;
    }
    printf("\n\n");

    for (int i = 0; i < arrlen; i++) {
        printf("%d\n", arr[i]);
    }
    printf("\n\n");
}

