#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "oscillator.h"
#include "breakpoints.h"
#include "audio_player.h"


/* Internal function declarations */
static int audio_player_callback(
        const void* inputBuffer,
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData);




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
AudioPlayer* new_audio_player(char* outfilename, int samplerate) {
    AudioPlayer* player = (AudioPlayer*) malloc(sizeof(AudioPlayer));
    if(player == NULL) {
        printf("Error allocating AudioPlayer\n");
        return NULL;
    }

    player->osc_list = NULL;
    player->samplerate = samplerate;


    //If outfile is NULL, disable file output
    if(outfilename == NULL)
        player->write_output = 0;
    else { // Otherwise, setup libsndfile file
        player->write_output = 1;

        player->sfinfo.samplerate = samplerate;
        player->sfinfo.channels = 1;
        // A float-based wave file
        player->sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        // Open file pointer
        if((player->outfile = sf_open(outfilename, SFM_WRITE, &player->sfinfo)) == NULL) {
            printf("Error opening output file.\n");
            puts(sf_strerror(NULL));
            free(player);
            return NULL;
        }
    }

    PaError err;

    // Initialize PortAudio
    if((err = Pa_Initialize()) != paNoError) {
        printf("PortAudio init error: %s\n", Pa_GetErrorText(err));
        free(player);
        return NULL;
    }

    // Choose default output device
    PaStreamParameters outputParameters;
    if((outputParameters.device = Pa_GetDefaultOutputDevice()) == paNoDevice) {
        printf("PortAudio: no default output device\n");
        free(player);
        return NULL;
    }

    // Set stream output settings
    outputParameters.channelCount = 1;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;

    // Open PortAudio stream with our callback. Pass a pointer to this AudioPlayer
    // as the callback's user data.
    err = Pa_OpenStream(&player->stream, NULL, &outputParameters, samplerate,
            paFramesPerBufferUnspecified, 0, audio_player_callback, player);
    if(err != paNoError) {
        printf("Error opening PortAudio stream: %s\n", Pa_GetErrorText(err));
        free(player);
        return NULL;
    }


    return player;
}





/* add_osc():
 * Adds an oscillator with the given settings to the specified AudioPlayer.
 *
 * The oscillator will be freed when it's removed from the oscillator list,
 * or when the AudioPlayer is freed.
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
        float waittime)
{

    /* To avoid the callback accessing the oscillator halfway through creation,
     * lock the oscillator_list */
    pthread_mutex_lock(&player->osc_list_lock);

    Oscillator* osc = new_osc(id, tab, tablen, bp, player->samplerate, freq, amplitude, length, waittime);
    oscil_list_add(&player->osc_list, osc);

    pthread_mutex_unlock(&player->osc_list_lock); //Unlock when done
}



/*
 * audio_player_callback():
 * A PortAudio callback function that generates audio data, writes it to the
 * output buffer, and writes it to the output file.
 *
 * inputBuffer:     Buffer containing any recorded data, none here
 * outputBuffer:    Buffer to fill with output samples
 * framesPerBuffer: How many frames are in the input & output buffers
 * timeInfo:        PortAudio time info
 * statusFlags:     PortAudio callback status flags
 * userData:        Custom data passed to the callback. Here, it's a
 *                  pointer to the associated AudioPlayer
 *
 */
static int audio_player_callback(
        const void* inputBuffer, 
        void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData) {


    // Get AudioPlayer from data
    AudioPlayer* player = (AudioPlayer*) userData;

    // Cast output buffer to float 
    float* out = (float*) outputBuffer;


    // Lock the oscillator list so the list isn't changed until we're done
    pthread_mutex_lock(&player->osc_list_lock);

    OscilNode* curr;
    float val;
    // For each sample
    for(int i = 0; i < framesPerBuffer; i++) {
        val = 0;
        
        // Tick each oscillator in list and sum their values
        curr = player->osc_list;
        while(curr != NULL) {
            val += oscil_tick(curr->osc);
            curr = curr->next;
        }

        // The output sample is the summed oscillator values
        out[i] = val;
    }

    // Done, so unlock the oscillator list
    pthread_mutex_unlock(&player->osc_list_lock);

    // If enabled, write to output file
    if(player->write_output)
        sf_write_float(player->outfile, out, framesPerBuffer);

    return 0;
}



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
void synch_update(AudioPlayer* player) {
    //Iterate through osc_list
    OscilNode* curr = player->osc_list;
    while(curr != NULL) {
        //If oscillator is expired, remove it
        if(oscil_expired(curr->osc)) {
            OscilNode* temp = curr->next;

            oscil_list_remove(&player->osc_list, curr->osc->id);

            curr = temp;
        }
        else
            curr = curr->next;
    }
}



/*
 * start_stream():
 * Starts the AudioPlayer generating audio, playing it back, and writing it to
 * its output file. Once the stream is started, it should be closed before
 * freeing the AudioPlayer.
 *
 * player:      The AudioPlayer to start
 *
 */
int start_stream(AudioPlayer* player) {
    PaError err;
    if((err = Pa_StartStream(player->stream)) != paNoError) {
        printf("Error starting stream: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    return 0;
}


/*
 * stop_stream():
 * Stops the AudioPlayer from generating audio, playing it back, and writing it to
 * its output file. If the stream is open, it should be closed before freeing
 * the AudioPlayer.
 *
 * player:      The AudioPlayer to start
 *
 */
int stop_stream(AudioPlayer* player) {
    PaError err;
    if((err = Pa_StopStream(player->stream)) != paNoError) {
        printf("Error stopping stream: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    return 0;
}



/*
 * free_audio_player():
 * Frees all allocated resources in the given AudioPlayer. Also frees the
 * pointer passed in.
 * 
 * Assumes the PA stream is stopped.
 *
 * player:      The AudioPlayer to free
 */
void free_audio_player(AudioPlayer* player) {
    // Free each oscillator remaining in the list
    OscilNode* curr = player->osc_list;
    while(curr != NULL) {
        OscilNode* temp = curr->next;

        oscil_list_remove(&player->osc_list, curr->osc->id);

        curr = temp;
    }

    // Close the output file
    sf_close(player->outfile);


    // Close the stream
    PaError err;
    if((err = Pa_CloseStream(player->stream)) != paNoError) {
        printf("Error closing stream: %s\n", Pa_GetErrorText(err));
    }

    // Destroy PortAudio
    if((err = Pa_Terminate()) != paNoError) {
        printf("PortAudio Termination error: %s\n", Pa_GetErrorText(err));
    }

    // Free the overall AudioPlayer
    free(player);
}


