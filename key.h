#ifndef KEY_H
#define KEY_H

/*
 * Key:
 * Holds the frequencies associated with a given key signature/scale
 */
typedef struct key {
    float* freqs;
    int len; //Length of the array
} Key;


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
Key* load_key(char* filename);


/*
 * free_key():
 * Frees the array contained in this struct. Also frees the struct pointer
 * itself.
 *
 * key:         A pointer to the Key struct to free.
 */
void free_key(Key* key);

#endif
