/** Assignment 3: Pointers and Functions
 * Objective: Learn how to pass pointers to functions.
 * Task:
 * Write a function swap(int *a, int *b) that swaps two integer values using pointers.
 * In the main() function, call swap() and pass the addresses of two integers.
 * Print the values of the integers before and after the swap.
**/

#include <stdio.h>

void swap (int *a, int *b) {
	*a = *a + *b;
	*b = *a - *b;
	*a = *a - *b;
}

int main() {
	int num1 = 5;
	int num2 = 8;

	swap(&num1, &num2);
	printf("%d\n%d\n", num1, num2);
}

