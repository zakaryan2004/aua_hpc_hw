/**
 * Assignment 7: Array of Pointers
 * Objective: Work with an array of pointers.
 * Task:
 * Declare an array of strings using an array of character pointers.
 * Print each string using pointer notation.
 * Modify one of the strings and print the updated array.
**/

#include <stdio.h>

int str_length(char *str) {
    int len = 0;

    while (*(str + len) != '\0') {
        len++;
    }

    return len;
}

int main() {
    char *str_arr_p[] = {"Hello", "world", "how", "are", "you"};

    for (int i = 0; i < 5; i++) {
        int strlen = str_length(*(str_arr_p + i));
        for (int j = 0; j < strlen; j++) {
            printf("%c", *(*(str_arr_p + i) + j)); // *(str_arr_p[i] + j)
        }       
        printf("\n");
    }
    printf("\n");

    str_arr_p[1] = "my dear world";

    for (int i = 0; i < 5; i++) {
        int strlen = str_length(str_arr_p[i]);
        for (int j = 0; j < strlen; j++) {
            printf("%c", str_arr_p[i][j]);
        }
        printf("\n");
    }

    return 0;
}

