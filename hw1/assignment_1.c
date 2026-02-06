/**
 * Assignment 1: Basics of Pointers
 * Objective: Understand the basics of pointers, address-of operator (&), and dereferencing(*).
 * Task:
 * Declare an integer variable and initialize it with a value.
 * Declare a pointer variable that points to the integer.
 * Print the address of the integer variable using both the variable and the pointer.
 * Modify the value of the integer using the pointer and print the new value.
*/

#include <stdio.h>

int main() {
    int num = 5;
    int *p = &num;
    
    printf("%p\n", &num);
    printf("%p\n", p);

    *p = 10;
    printf("%d\n", num);
    printf("%d\n", *p);
}

