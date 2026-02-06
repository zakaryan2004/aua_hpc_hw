/**
 * Assignment 6: String Manipulation with Pointers
 * Objective: Work with strings using pointers.
 * Task:
 * Declare a character pointer and assign it a string literal.
 * Use a pointer to traverse and print the string character by character.
 * Write a function str_length(char *str) that calculates the length of a string using pointer arithmetic.
 * Call str_length() in main() and print the length of a user-provided string.
**/

#include <stdlib.h>
#include <stdio.h>

int str_length(char *str) {
    int len = 0;

    while (*(str + len) != '\0') {
        len++;
    }

    return len;
}

int main() {
    char *ch_p = "Hello!";

    for (int i = 0; i < 6; i++) {
        printf("%c", *(ch_p + i));
    }

    printf("%d\n", str_length(ch_p));

    return 0;
}

