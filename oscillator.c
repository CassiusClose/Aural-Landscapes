#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "oscillator.h"


/*********************
 * OSCILLATOR STRUCT *
 *********************/


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
        float waittime)
{
    Oscillator* osc = (Oscillator*) malloc(sizeof(Oscillator));
    osc->id = id;
    osc->tab = tab;
    osc->tablen = tablen;
    osc->vol_bp = vol_bp;
    osc->samplerate = samplerate;
    osc->freq = freq;
    osc->amplitude = amplitude;
    osc->slength = length*samplerate; //Convert length in seconds to samples

    osc->index = 0;
    osc->curr_t = 0;

    /* Audio starts at sample 0, so start the current sample at negative the
     * time before audio starts */
    osc->curr_sample = -waittime*samplerate;

    /* The lookup table holds one period, so tablen/samplerate is how much to
     * increase the table index for a frequency of 1. Then multiply by freq */
    osc->inc = freq * tablen / samplerate;

    osc->tinc = 1.0 / samplerate;

    return osc;
}



/*
 * oscil_tick():
 * Returns the wave amplitude value for the Oscillator's current sample, increments
 * the Oscillator to its next sample.
 *
 * osc:         The oscillator to tick
 *
 * return:      The amplitude value for the current sample 
 */
float oscil_tick(Oscillator* osc) {
    // If sample out of range (before 0 or past the oscillator's length), return 0
    if(osc->curr_sample < 0 || osc->curr_sample > osc->slength) {
        osc->curr_sample++;
        return 0;
    }

    // Increment sample
    osc->curr_sample++;

    // Get the sample value from the table
    float val = osc->amplitude * osc->tab[osc->index];
    
    // Recalculate the increment value 
    osc->inc = osc->freq * osc->tablen / (float) osc->samplerate;

    // Update the table index, keep it below the table length
    osc->index += osc->inc;
    if(osc->index > osc->tablen)
        osc->index -= osc->tablen;

    // Update current time (relative to sample 0)
    osc->curr_t += osc->tinc;


    // Figure out what percent of the way the oscillator is through its length
    float perc = osc->curr_sample/(float)osc->slength;

    // Stretch breakpoint file to cover entire oscillator length,
    // adjust the sample value with the breakpoint's amplitude value
    float amp = get_percentval(osc->vol_bp, perc);
    val = val*amp;

    return val;
}



/*
 * oscil_expired():
 * Returns whether the Oscillator's current sample has surpassed the length of the
 * oscillator, i.e. no more sound is being generated.
 *
 * osc:         The Oscillator in question
 *
 * return:      Boolean, whether the Oscillator has expired
 */
int oscil_expired(Oscillator* osc) {
    if(osc->curr_sample > osc->slength)
        return 1;
    return 0;
}



/*
 * oscil_free():
 * Frees some resources held by the given Oscillator. Also frees the passed pointer.
 * To be able to reuse Breakpoints and lookup tables, doesn't free either of those.
 * They will need to be freed seperately.
 *
 * osc:         The Oscillator to free
 * 
 */
void oscil_free(Oscillator* osc) {
    free(osc);
}





/**************************
 * OSCILLATOR LINKED LIST *
 **************************/


/*
 * oscil_list_add():
 * Adds the given Oscillator to the end of the given linked list.
 *
 * head:        A double pointer to the head of the list
 * osc:         A pointer to the Oscillator to add
 */
void oscil_list_add(OscilNode** head, Oscillator* osc) {
    OscilNode* node = (OscilNode*) malloc(sizeof(OscilNode));
    node->osc = osc;

    if(*head == NULL) {
        *head = node;
        node->next = NULL;
        node->prev = NULL;
    }
    else { 
        OscilNode* curr = *head;
        while(curr->next != NULL)
            curr = curr->next;
        curr->next = node;
        node->prev = curr;
        node->next = NULL;
    }
}


/*
 * oscil_list_remove():
 * Removes and frees the oscillator with the given id from the given list.
 *
 * head:        A double pointer to the head of the list
 * id:          The id of the oscillator to remove
 */
void oscil_list_remove(OscilNode** head, int id) {
    OscilNode* curr = *head;

    if(*head == NULL)
        return;

    if(curr->osc->id == id) {
        if(curr->next)
            curr->next->prev = NULL;
        OscilNode* temp = curr->next;

        oscil_free(curr->osc);
        free(curr);

        *head = temp;
        return;
    }
    

    while(curr != NULL) {
        if(curr->osc->id == id) {
            if(curr->prev)
                curr->prev->next = curr->next; 
            if(curr->next)
                curr->next->prev = curr->prev;

            oscil_free(curr->osc);
            free(curr);
            return;
        }

        curr = curr->next;
    }
}


/*
 * oscil_list_free():
 * Frees the given list and any oscillators in it.
 *
 * head:        A pointer to the head of the list
 *
 */
void oscil_list_free(OscilNode* head) {
    OscilNode* temp;
    while(head != NULL) {
        temp = head->next;
        if(head->osc != NULL)
            oscil_free(head->osc);
        free(head);
        head = temp;
    }
}




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
float* gen_sin_tab(int len) {
    float* table = (float*) malloc(sizeof(float)*len);

    float c = 2 * M_PI / len;
    for(int i = 0; i < len; i++)
        table[i] = sin(c*i);

    return table;
}


/*
 * gen_square_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a square wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_square_tab(int len) {
    float* table = (float*) malloc(sizeof(float)*len);

    int i = 0;
    for(i = 0; i < len/2; i++)
        table[i] = 1;
    for(; i < len; i++)
        table[i] = -1;

    return table;
}


/*
 * gen_sawtooth_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a sawtooth wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_sawtooth_tab(int len) {
    float* table = (float*) malloc(sizeof(float)*len);
    
    for(int i = 0; i < len; i++) {
        table[i] = 0;
        for(int j = 0; j < 100; j++) {
            int sign = 1;
            if((j+1) % 2 != 0)
                sign = -1;
            table[i] += -(2/(M_PI*(j+1)))*sign * sin(2*M_PI*(j+1)*i/(float)len);
        }
    }
    return table;
}


/*
 * gen_triangle_tab():
 * Generates and returns a malloc'ed lookup table at the given length containing
 * one period of a triangle wave. The lookup table must be freed. 
 *
 * len:         The length of the table to generate
 *
 * return:      A malloc'ed lookup table
 */
float* gen_triangle_tab(int len) {
    float* table = (float*) malloc(sizeof(float)*len);

    for(int i = 0; i < len/2; i++) {
        table[i] = -1 + 2*i/(float)(len/2-1);
    }
    for(int i = len/2; i < len; i++) {
        table[i] = 1 - 2*(i-len/2)/(float)(len/2);
    }

    return table;

}


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
float* gen_fourier_tab(int len, float amps[10]) {
    float* table = (float*) malloc(sizeof(float)*len);
        
    for(int i = 0; i < len; i++) {
        table[i] = 0;
        for(int j = 0; j < 10; j++)
            table[i] += amps[j] * sin(2*M_PI*(j+1)*i/(float)len);
    }

    return table;
}


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
float* gen_warmth_tab(int len, int temp) {
    if(temp == 1) {
        float amps[10] = {0.8, 0.2, 0, 0, 0, 0, 0, 0, 0};
        return gen_fourier_tab(len, amps);
    } else if(temp == 2) {
        float amps[10] = {0.6, 0.3, 0.05, 0.05, 0, 0, 0, 0, 0};
        return gen_fourier_tab(len, amps);
    } else if(temp == 3) {
        float amps[10] = {0.4, 0.35, 0.1, 0.05, 0.04, 0, 0, 0, 0};
        return gen_fourier_tab(len, amps);
    } else if(temp == 4) {
        float amps[10] = {0.2, 0.4, 0.15, 0.1, 0.05, 0.04, 0, 0, 0};
        return gen_fourier_tab(len, amps);
    } else if(temp == 5) {
        float amps[10] = {0.15, 0.3, 0.3, 0.2, 0.025, 0.02, 0.005, 0, 0};
        return gen_fourier_tab(len, amps);
    } else if(temp == 6) {
        float amps[10] = {0.1, 0.15, 0.15, 0.3, 0.05, 0.03, 0.02, 0.005, 0.005};
        return gen_fourier_tab(len, amps);
    } else if(temp == 7) {
        float amps[10] = {0.05, 0.08, 0.1, 0.15, 0.2, 0.1, 0.08, 0.02, 0.01};
        return gen_fourier_tab(len, amps);
    } else {
        float amps[10] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
        return gen_fourier_tab(len, amps);
    }
}
