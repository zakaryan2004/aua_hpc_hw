#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DNA_LENGTH 1024 * 1024 * 256 // 256MiB

int main() {
    srand(time(0));
    const char file_name[] = "dna.txt";
    const char dna_symbols[] = "ACGT";
    FILE *f_out = fopen(file_name, "w");
    
    if (f_out == NULL) {
        perror("fopen failed");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < DNA_LENGTH; i++) {
        fprintf(f_out, "%c", dna_symbols[rand() % 4]);   
    }

    fclose(f_out);

    return 0;
}
