#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DNA_LENGTH 1024 * 1024 * 256 // 256MiB

int main() {
    srand(time(0));
    const char file_name[] = "random_text.txt";
    FILE *f_out = fopen(file_name, "w");
    
    if (f_out == NULL) {
        perror("fopen failed");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < DNA_LENGTH; i++) {
        // ASCII printable characters: 32-126
        // char ch = rand() % (126 - 32) + 32;
        char ch = rand() % 126 + 1;
        if (ch < 32) ch = 32; // make spaces (32) more frequent
        fprintf(f_out, "%c", ch);   
    }

    fclose(f_out);

    return 0;
}
