#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include "breakpoints.h"

/*
 * Oscillator:
 * Generates periodic audio data one sample at a time based on a many different
 * settings.
 *
 * The oscil_tick() returns the audio's amplitude at the current sample and
 * increments the Oscillator to the next sample. This is the backbone of this
 * struct. User functions should set up the Oscillator how they want, and then
 * call oscil_tick() for every sample.
 *
 * The waveform of the audio data is stored in a precomputed lookup table.
 *
 * The amplitude of the waveform in the lookup table is controlled by a Breakpoints
 * struct.
 *
 */
typedef struct oscillator {
    int id; // An id associated with this oscillator (unique for a given oscillator list)

    float* tab; // Lookup table holding audio data
    int tablen; // Length of the lookup table

    Breakpoints* vol_bp; // Amplitude controlling Breakpoints

    float freq; // The frequency to generate the audio at
    float amplitude; // The base amplitude, which the breakpoints will adjust

    float slength; // How long the audio should play (in samples)

    int samplerate; // The sample rate to generate audio at

    int curr_sample; // The current sample. Can be negative. Audio starts at 0 
    float curr_t; // The current time value, relative on sample 0

    int index; // The current sample's index into the lookup table
    int inc; // How much to increment index by every sample
    float tinc; // How much to increment the current time by every sample
} Oscillator;


/* 
 * new_osc():
 * Creates a malloc'ed Oscillator struct with the given settings, and values
 * initalized.
 *
 * When done with this Oscillator, must call oscil_free() to free all resources.
 *
 * id:          An id, unique for any list this oscillator might be in
 * tab:         A pointer to the lookup table, holds one period of the wave
 * tablen:      The length of the given lookup table
 * vol_bp:      The Breakpoints to control Oscillator amplitude
 * samplerate:  The rate to sample the wave
 * freq:        The frequency at which to generate the wave
 * amplitude:   The base amplitude of the Oscillator
 * length:      How long the audio should play for (in seconds)
 * waittime:    How long the Oscillator should wait before beginning (in seconds)
 *
 * return:      A malloc'ed pointer to the created Oscillator.
 */
Oscillator* new_osc(
        int id, 
        float* tab, 
        int tablen, 
        Breakpoints* vol_bp, 
        int samplerate, 
        float freq, 
        float amplitude, 
        float length, 
        float waittime);


/*
 * oscil_tick():
 * Returns the wave amplitude value for the Oscillator's current sample, increments
 * the Oscillator to its next sample.
 *
 * osc:         The oscillator to tick
 *
 * return:      The amplitude value for the current sample 
 */
float oscil_tick(Oscillator* osc);

/*
 * oscil_expired():
 * Returns whether the Oscillator's current sample has surpassed the length of the
 * oscillator, i.e. no more sound is being generated.
 *
 * osc:         The Oscillator in question
 *
 * return:      Boolean, whether the Oscillator has expired
 */
int oscil_expired(Oscillator* osc);


/*
 * oscil_free():
 * Frees some resources held by the given Oscillator. Also frees the passed pointer.
 * To be able to reuse Breakpoints and lookup tables, doesn't free either of those.
 * They will need to be freed seperately.
 *
 * osc:         The Oscillator to free
 * 
 */
void oscil_free(Oscillator* osc);






/* Generate Oscillator Lookup Tables */

/*
 * gen_sin_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a sine wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_sin_tab(int len);

/*
 * gen_square_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a square wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_square_tab(int len);

/*
 * gen_sawtooth_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a sawtooth wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_sawtooth_tab(int len);

/*
 * gen_triangle_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a triangle wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_triangle_tab(int len);

/*
 * gen_fourier_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a wave with the given Fourier Coefficients. The lookup table
 * must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_fourier_tab(int len, float amps[10]);


/*
 * gen_warmth_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a wave. The lookup table must be freed. 
 *
 * The given temperature determines the content of the wave. A higher temperature
 * value generates a "cooler" sound with more higher harmonics. Supports
 * tempatures in the range 0-7. Any other value will return a wave with just the
 * base harmonic.
 *
 * len:         The length of the table to generate
 * temp:        Higher values generates waves with higher harmonics. Supports 0-7
 *
 * return:      A malloc'ed lookup table
 */
float* gen_warmth_tab(int len, int temp);














/*
 * OscilNode:
 * A node for a linked list of Oscillators
 *
 */
typedef struct oscil_node {
    struct oscil_node* next; // Next item in the list
    struct oscil_node* prev; // Prev item in the list

    Oscillator* osc; // The Oscillator associated with this node
} OscilNode;



/*
 * oscil_list_add():
 * Adds the given Oscillator to the end of the given linked list.
 *
 * head:        A double pointer to the head of the list
 * osc:         A pointer to the Oscillator to add
 */
void oscil_list_add(OscilNode** head, Oscillator* osc);

/*
 * oscil_list_free():
 * Frees the given list and any oscillators in it.
 *
 * head:        A pointer to the head of the list
 *
 */
void oscil_list_free(OscilNode* head);

/*
 * oscil_list_remove():
 * Removes and frees the oscillator with the given id from the given list.
 *
 * head:        A double pointer to the head of the list
 * id:          The id of the oscillator to remove
 */
void oscil_list_remove(OscilNode** head, int id);

#endif
