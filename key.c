#include "key.h"

#include <stdlib.h>
#include <stdio.h>

/*
 * load_key():
 * Reads in a file of frequencies and saves them in a malloc'ed Key struct.
 * The file must contain one frequency value per line and nothing else.
 *
 * The user must call free_key() on this struct.
 *
 * filename:    The file to read
 *
 * return:      A malloc'ed Key struct
 */
Key* load_key(char* filename) {
    Key* key = (Key*) malloc(sizeof(Key));
    FILE* f = fopen(filename, "r");

    int len = 0;
    char c;
    /* First read through the file, figure out its length */
    while((c = fgetc(f)) != EOF) {
        if(c == '\n')
            len++;
    }
    
    // Go to beginning of file
    rewind(f);

    key->freqs = (float*) malloc(sizeof(float) * len);

    /* Now scan in each freq. */
    for(int i = 0; i < len; i++) {
        fscanf(f, "%f", key->freqs+i);
    }

    fclose(f);

    key->len = len;

    return key;
}


/*
 * free_key():
 * Frees the array contained in this struct. Also frees the struct pointer
 * itself.
 *
 * key:         A pointer to the Key struct to free.
 */
void free_key(Key* key) {
    free(key->freqs);
    free(key);
}
