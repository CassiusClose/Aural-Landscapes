#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <sndfile.h>
#include <portaudio.h>

#include "oscillator.h"


/*
 * AudioPlayer:
 * Contains:
 * - A collection of Oscillators to generate audio
 * - PortAudio systems to play the audio in realtime
 * - libsndfile systems to write the file in realtime
 *
 * See oscillator.h for more information on how Oscillators generate audio.
 *
 */
typedef struct audio_player {
    /**** Audio Generation (Oscillator) ****/

    // A list of Oscillators from which to generate and combine audio
    OscilNode* osc_list;

    /* osc_list is accessed by the asynchronous PortAudio callback,
     * and adding an oscillator to the list can take a decent bit of time,
     * so use this mutex lock to prevent the callback from accessing osc_list
     * in the middle of adding an Oscillator.
     *
     * Internally, this should be locked whenever the list is being modified
     * and in the callback function. */
    pthread_mutex_t osc_list_lock;



    /**** File output (libsndfile) ****/
    /* Note: At least on my computer, writing to a file can take up enough
     * resources to cause buffer underruns. This is why it can be disabled. */

    int write_output; // Boolean, whether or not to write to an output file
    SF_INFO sfinfo; // libsndfile info about the output file
    SNDFILE* outfile; // libsndfile file pointer to the output file



    /**** Audio streaming (PortAudio) ****/
    // The PortAudio stream that calls the callback function
    PaStream* stream;



    /**** Settings ****/
    int samplerate; // Sample rate to generate audio at
} AudioPlayer;







/* 
 * new_audio_player(): 
 * Creates a malloc'ed AudioPlayer struct with the given settings, initalizes
 * its audio stream, and opens its output file if specified
 *
 * When done with this AudioPlayer, the user must call free_audio_player() to
 * free its resources.
 *
 * outfilename: The name of the file to write audio to. If NULL, no file will
 *              be written
 *
 * return:      A malloc'ed pointer to an initialized AudioPlayer with the given
                settings and
 */
AudioPlayer* new_audio_player(char* outfilename, int samplerate);


/* add_osc():
 * Adds an oscillator with the given settings to the specified AudioPlayer.
 *
 * player:      The AudioPlayer to add the oscillator to
 * id:          The Oscillator's id
 * tab:         A pointer to the Oscillator's lookup-table
 * tablen:      The length of the lookup-table
 * bp:          A pointer to the Oscillator's Breakpoint struct
 * freq:        The oscillator's frequency
 * amplitude:   The oscillator's base amplitude
 * length:      The oscillator's length in seconds
 * waittime:    The oscillator's waittime in seconds
 *
 */
void add_osc(
        AudioPlayer* player,
        int id,
        float* tab,
        int tablen, 
        Breakpoints* bp, 
        float freq, 
        float amplitude, 
        float length, 
        float waittime);

/*
 * free_audio_player():
 * Frees all allocated resources in the given AudioPlayer. Also frees the
 * pointer passed in.
 *
 * Assumes that the stream is stopped (stop_stream() has been called if
 * the stream was ever started)
 *
 * player:      The AudioPlayer to free
 */
void free_audio_player(AudioPlayer* player);


/*
 * synch_update():
 * Updates the AudioPlayer's oscillator list, removing expired Oscillators.
 *
 * This takes too much time to be called by the callback function, so the user
 * program should occasionally call this to keep the oscillator list from
 * getting too long.
 *
 * player:      The AudioPlayer to update
 *
 */
void synch_update(AudioPlayer* player);


/*
 * start_stream():
 * Starts the AudioPlayer generating audio, playing it back, and writing it to
 * its output file. Once the stream is started, it should be closed before
 * freeing the AudioPlayer.
 *
 * player:      The AudioPlayer to start
 *
 */
int start_stream(AudioPlayer* player);

/*
 * stop_stream():
 * Stops the AudioPlayer from generating audio, playing it back, and writing it to
 * its output file. If the stream is open, it should be closed before freeing
 * the AudioPlayer.
 *
 * player:      The AudioPlayer to start
 *
 */
int stop_stream(AudioPlayer* player);


#endif
