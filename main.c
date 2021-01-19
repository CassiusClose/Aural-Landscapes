/*****************************************************************************
 * Aural Landscapes
 * An image-based algorithmic composition program.
 *
 * Cassius Close, cclose@u.rochester.edu
 * 
 * A school project for Audio Software Design I (AME262) at the University of
 * Rochester.
 *
 * The inital ideas for some of my structs were taken from class example
 * programs, which have their own acknowledgements.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <portaudio.h>
#include <pthread.h>
#include <string.h>

#include "audio_player.h"
#include "breakpoints.h"
#include "image.h"
#include "key.h"


/* Include platform specific libraries that control terminal input, for use with
 * enable_special_input() (if UNIX-based) or getchar_immediate() (if Windows) */
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <fcntl.h>
#endif


#ifdef USE_GRAPHICS
/* If graphics are supported, then graphics.h holds a struct that holds SDL objects */
#include "graphics.h"
#endif





/*******************************
 * DECLARES & GLOBAL VARIABLES *
 *******************************/


#define SAMPLE_RATE 48000

// The size of the region of pixels to analyze at one time
#define RECT_WIDTH 50
#define RECT_HEIGHT 50


#ifdef USE_GRAPHICS
/* A boolean that keeps track of whether the program should close. If graphics
 * are enabled, another thread is spawned that detects if the SDL window is
 * closed. This is the way that the thread communicates with the main thread to
 * say that the window has been closed.
 */
int should_close = 0;
#endif






/*************************
 * FUNCTION DECLARATIONS *
 *************************/

// Mathy functions
float randfloat(float beg, float end);
int randint(int beg, int end);
float percent_in_range(float perc, float beg, float end);

// Detecting quit functions
int shouldClose();
#ifdef USE_GRAPHICS
void* detect_close(void *vargp);
#endif

// "press-any-button-to-quit" mode
char getchar_immediate();
void enable_special_input();
void disable_special_input();

// Misc
void usage();


/*
 * aural_landcapes:
 * An algorithmic composition program that generates realtime ambient music from
 * image files.
 *
 * Every few seconds, chooses a new region from the image and generates audio
 * from those pixels' color data.
 *
 * Depends on PortAudio, libsndfile, and LodePNG. If compiled with the optional
 * graphics mode, USE_GRAPHICS is defined, and this program depends on SDL2.
 *
 * Graphics mode will display the image in a window, and will mark the currently
 * selected region of pixels with a white rectangle.
 *
 * Currently, only .png image files are supported.
 *
 *
 * -----Command line arguments------:
 * ./aural_landscapes input.png [-o output.wav] [--hide_rect]
 *
 * input.png:                   filepath to the image file to use
 *
 * -o output.wav (optional):    if an output file is specified, will write the
 *                              generated audio data into that output file.
 *
 *  --hide-rect (optional):     if graphics mode is enabled, will not display the
 *                              rectangle that marks the currently selected region
 *
 */
int main(int argc, char** argv) {
    /*********************
     * COMMAND-LINE ARGS *
     *********************/


    /* Not enough command line arguments */
    if(argc < 2) {
        usage();
        return 1;
    }

    // The input image file
    char* input_filename = argv[1];

    // Output filename, NULL if not write enabled
    char* output_filename = NULL;

    #ifdef USE_GRAPHICS
    // Whether or not to display the currently selected region on the image
    int hide_rect = 0;
    #endif

    /* Process command line arguments */
    for(int i = 2; i < argc; i++) {
        // Should output to a file
        if(strcmp(argv[i], "-o") == 0) {
            // Not another argument for the output filename provided
            if(i+1 == argc) {
                usage();
                printf("\nMust provide output filename for -o\n");
                return 1;
            }

            output_filename = argv[++i];
        }
        #ifdef USE_GRAPHICS
        // Should hide the rectangle on the image
        else if(strcmp(argv[i], "--hide-rect") == 0) {
            hide_rect = 1; 
        }
        #endif
        else {
            usage();
            printf("\nUnrecognized flag: %s\n", argv[i]);
            return 1;
        }
    }



    /**********************
     * INITIALIZE STRUCTS *
     **********************/


    printf("Initializing...\n");

    srand(time(NULL));

    // Load image pixels 
    unsigned int imagew, imageh;
    unsigned char* rawpix = load_imagefile(input_filename, &imagew, &imageh);
    if(rawpix == NULL) {
        printf("Error loading image file.\n");
        return 1;
    }

    /* Load image into Image struct */
    Image* image = load_to_image(rawpix, imagew, imageh);
    if(image == NULL) {
        printf("Error loading image... quitting\n");
        free(rawpix);
        return 1;
    }


    /*
    // Unix only: Redirect stderr to avoid ALSA error messages
    #ifndef _WIN32
    freopen("/dev/null", "w", stderr);
    #endif
    */



    #ifdef USE_GRAPHICS
    /* Initialize graphics */

    // Initialize SDL structs
    Graphics* graphics = create_graphics("Aural Landscapes", imagew, imageh);
    if(graphics == NULL) {
        printf("Error loading SDL graphics... quitting\n");
        free(rawpix);
        free_image(image);
        return 1;
    }
    
    // Convert char pixels into Uint32, the format SDL uses
    Uint32* pixels = convert_rgba_ints_to_Uint32(rawpix, imagew*imageh*4);

    // Display the image and reload the window
    setPixels(graphics, pixels, imagew, imageh);
    updateWindow(graphics);


    /* SDL doesn't seem to save the fact that someone clicked the window's
     * close button, so spawn another thread that continually checks.
     * See detect_close() */
    pthread_t tid;
    pthread_create(&tid, NULL, detect_close, NULL);
    #endif



    /* Load the amplitude breakpoint file for the Oscillators */
    Breakpoints* bp = load_bp_file("resources/bps/bp2.txt");
    if(bp == NULL) {
        printf("Error loading breakpoint file... quitting\n");
        free(rawpix);
        free_image(image);
        #ifdef USE_GRAPHICS
        free_graphics(graphics);
        #endif
        return 1;
    }



    /* Initialize the audio player struct (PA and libsndfile) */
    AudioPlayer* player = new_audio_player(output_filename, SAMPLE_RATE);
    if(player == NULL) {
        printf("Error loading audio player... quitting\n");
        free(rawpix);
        free_image(image);
        #ifdef USE_GRAPHICS
        free_graphics(graphics);
        #endif
        free_breakpoints(bp);
        return 1;
    }



    /* Load lookup tables */
    int tablen = SAMPLE_RATE; //Table length as SR supports freqs down to 1

    int NUM_TABS = 8;
    float* tabs[8] = {
        gen_warmth_tab(tablen, 7),
        gen_warmth_tab(tablen, 6),
        gen_warmth_tab(tablen, 5),
        gen_warmth_tab(tablen, 4),
        gen_warmth_tab(tablen, 3),
        gen_warmth_tab(tablen, 2),
        gen_warmth_tab(tablen, 1),
        gen_warmth_tab(tablen, 0)
    };
    /* Make sure none failed loading */
    int err = 0;
    // See if any failed to load
    for(int i = 0; i < NUM_TABS; i++) {
        if(tabs[i] == NULL) {
            err = 1;
            break;
        }
    }
    // If any failed loading, go through and free all of them, then free
    // everything and exit
    if(err) {
        for(int i = 0; i < NUM_TABS; i++) {
            if(tabs[i] != NULL)
                free(tabs[i]);
        }
        printf("Error loading table... quitting\n");
        free(rawpix);
        free_image(image);
        #ifdef USE_GRAPHICS
        free_graphics(graphics);
        #endif
        free_breakpoints(bp);
        free_audio_player(player);
        return 1;
    }



    /* Load major keys */
    int MAJOR_KEYS_LEN = 3;
    Key* major_keys[3];
    major_keys[0] = load_key("resources/keys/cmaj.txt");
    major_keys[1] = load_key("resources/keys/dmaj.txt");
    major_keys[2] = load_key("resources/keys/emaj.txt");

    err = 0;
    // See if any failed to load
    for(int i = 0; i < MAJOR_KEYS_LEN; i++) {
        if(major_keys[i] == NULL) {
            err = 1;
            break;
        }
    }
    // If any failed to load, free all of them and exit
    if(err) {
        printf("Error loading major key... quitting\n");
        for(int i = 0; i < MAJOR_KEYS_LEN; i++) {
            if(major_keys[i] != NULL)
                free(major_keys[i]);
            return 1;
        }
        free(rawpix);
        free_image(image);
        #ifdef USE_GRAPHICS
        free_graphics(graphics);
        #endif
        free_breakpoints(bp);
        free_audio_player(player);
        for(int j = 0; j < NUM_TABS; j++)
            free(tabs[j]);
    }



    /* Load harmonic keys */
    int HARMONIC_KEYS_LEN = 3;
    Key* harmonic_keys[3];
    harmonic_keys[0] = load_key("resources/keys/charm.txt");
    harmonic_keys[1] = load_key("resources/keys/dharm.txt");
    harmonic_keys[2] = load_key("resources/keys/eharm.txt");

    err = 0;
    // See if any failed to load
    for(int i = 0; i < HARMONIC_KEYS_LEN; i++) {
        if(harmonic_keys[i] == NULL) {
            err = 1;
            break;
        }
    }
    // If any failed to load, free all of them and exit
    if(err) {
        printf("Error loading harmonic key... quitting\n");
        for(int i = 0; i < HARMONIC_KEYS_LEN; i++) {
            if(harmonic_keys[i] != NULL)
                free(harmonic_keys[i]);
        }
        for(int i = 0; i < MAJOR_KEYS_LEN; i++)
            free(major_keys[i]);
        free(rawpix);
        free_image(image);
        #ifdef USE_GRAPHICS
        free_graphics(graphics);
        #endif
        free_breakpoints(bp);
        free_audio_player(player);
        for(int j = 0; j < NUM_TABS; j++)
            free(tabs[j]);
        return 1;
    }




    /* Pick a key based on the overall warmth of the image */
    Key* key;
    int tot_warmth = tot_avg_warmth(image);

    // If image is cold overall, choose a harmonic minor key
    if(tot_warmth < 0) {
        key = harmonic_keys[randint(0, HARMONIC_KEYS_LEN)];
    }
    // If image is cold overall, choose a major key
    else {
        key = major_keys[randint(0, MAJOR_KEYS_LEN)];
    }


    printf("Done\n\n");






    /*************
     * MAIN LOOP *
     *************/


    #ifndef USE_GRAPHICS
    // Enable "press any key to quit" mode
    printf("Press any key to quit...\n");
    enable_special_input();
    #endif

    // Unlock the AudioPlayer's list of oscillators for use
    pthread_mutex_unlock(&player->osc_list_lock);

    // Start PortAudio streaming
    start_stream(player);

    // Each oscillator has a unique id, increment this when you create one
    unsigned int oscID = 0;

    /* Loop until the user ends the program. For different settings, this means
     * different things. See shouldClose() for more details. Note that because this
     * loop sleeps at the end, there will be a delay between when the user presses
     * quit and the program actually quits. */
    while(!shouldClose()) {
        // Update the oscillator list (removes completed oscillators)
        synch_update(player);

        // Choose a random region of the image
        int startx = randint(0, imagew-RECT_WIDTH);
        int starty = randint(0, imageh-RECT_HEIGHT);

        #ifdef USE_GRAPHICS
        // If enabled, update the window to highlight the new region
        if(!hide_rect) {
            draw_rect(graphics, startx, starty, RECT_WIDTH, RECT_HEIGHT);
            updateWindow(graphics);
        }
        #endif


        /* Calculate the average brightness and warmth of this region
         * Average brightness is between 0 and 1 */
        float avg_brightness = avg_perc_brightness(image, startx, starty, RECT_WIDTH, RECT_HEIGHT);
        int avg_warm = avg_warmth(image, startx, starty, RECT_WIDTH, RECT_HEIGHT);

        /* Pick number of notes to generate
         * If the brightness is low, then the notes will be lower in frequency.
         * Generate fewer to avoid as much clashing between them. */
        int num_notes;
        if(avg_brightness < 0.5)
            num_notes = randint(1, 3);
        else
            num_notes = randint(1, 4);

        /* Generate each note */
        for(int i = 0; i < num_notes; i++) {

            /* Pick a brightness value near the calculated average. Ensure
             * brightness values is between 0 and 1. Then use this to choose
             * from the higher or lower end of the frequency list */
            float brightness = randfloat(0.5*avg_brightness, 1.2*avg_brightness);
            if(brightness > 1)
                brightness = 1;
            float freq = key->freqs[(int) percent_in_range(brightness, 0, key->len)];

            /* Higher notes (from brighter colors) tend to be louder, so
             * calculate an amplitude that decreases as brightness increases */
            float amp = (1-brightness) * 0.7 + 0.3;



            /* Pick a warmth value nearby the calculated average. Process the
             * warmth value so it's between 0 & 1, then use that to pick an
             * appropriate lookup table from the list of tables. */
            float warmth = randint(avg_warm-10, avg_warm+10);
            warmth -= 30; // Adjust where warmth maps to arr of harmonic content
            if(warmth < -100) // min value: -100
                warmth = -100;
            if(warmth > 100) // max value: 100
                warmth = 100;
            warmth += 100; // warmth between 0 and 200
            warmth /= 200.0; //warmth between 0 and 1

            int ind = NUM_TABS*warmth; // Lookup table index



            // Pick random future start time and note length
            float start = randfloat(0.1, 5);
            float len = randfloat(3, 10);

            // Add the note's oscillator to the list.
            // 0.4 is a hardcoded base amplitude so everything isn't really loud
            add_osc(player, oscID++, tabs[ind], tablen, bp, freq, 0.4*amp, len, start);
        }


        /* Sleep for 6 seconds before moving the image region and generating
         * new notes */
        Pa_Sleep(6000);
    }

    // Stop the PortAudio stream
    stop_stream(player);

    #ifndef USE_GRAPHICS
    // Disable the "press-any-key-to-quit" mode
    disable_special_input();
    #endif




    /******************
     * FREE RESOURCES *
     ******************/

    free(rawpix);
    free_image(image);

    free_breakpoints(bp);

    #ifdef USE_GRAPHICS
    free_graphics(graphics);
    #endif

    free_audio_player(player);

    for(int i = 0; i < NUM_TABS; i++)
        free(tabs[i]);

    for(int i = 0; i < MAJOR_KEYS_LEN; i++)
        free_key(major_keys[i]);
    for(int i = 0; i < HARMONIC_KEYS_LEN; i++)
        free_key(harmonic_keys[i]);


    return 0;    
}



/*******************
 * MATHY FUNCTIONS *
 *******************/

/*
 * randfloat():
 * Returns a random float in the range specified, including both ends of the range
 *
 * beg: the lower end of the range to generate from
 * end: the upper end of the range to generate from
 *
 * return: A randomly generated float in the range given
 */
float randfloat(float beg, float end) {
    return ((float) rand() / (float) RAND_MAX) * (end-beg) + beg;
}

/*
 * randint():
 * Returns a random integer in the range specified, not-including the end of the range.
 *
 * beg: the lower end of the range to generate from, inclusive
 * end: the higher end of the range to generate from, exclusive
 *
 * return: A randomly generated int in the range given
 */
int randint(int beg, int end) {
    return rand() % (end-beg) + beg;
}


/*
 * percent_in_range():
 * Returns a float that is a certain percentage through the given range.
 *
 * Ex: if the range was 0-10, perc=0.5 would return halfway through the range: 5
 *
 * perc: The percentage, between 0 and 1, of how far through the range we go
 * beg: The beginning of the range
 * end: The end of the range
 *
 * return: The float at a certain percentage of the way through the given range.
 */
float percent_in_range(float perc, float beg, float end) {
    return perc*(end-beg) + beg;
}




/**************************
 * PROGRAM QUIT DETECTION *
 **************************/

/*
 * shouldClose():
 * Returns whether or not the user has triggered program to quit.
 *
 * If graphics mode is enabled, quitting means the user has pressed the close button
 * on the graphical window. Otherwise, quitting means the user has pressed any key.
 *
 * return: a boolean representing whether the user has triggered the program to quit
 */
int shouldClose() {
    #ifdef USE_GRAPHICS
        // detect_close() lives on a seperate thread and sets this
        return should_close;
    #else
        return (getchar_immediate() != -1);
    #endif
}



#ifdef USE_GRAPHICS
/*
 * detect_close():
 * Detects whether the user has pressed the close button the SDL window.
 * Designed to be the callback function of a separate thread, since SDL
 * doesn't seem to save whether or not the close button has been pressed.
 *
 * vargp: void pointer to thread arguments. Here, should be pointer to Graphics
 *      struct
 *
 * return: 1 if the close button has been pressed, 0 otherwise
 */
void* detect_close(void* vargp) {
    while(1) {
        if(!isWindowOpen((Graphics*) vargp)) {
            should_close = 1;
            printf("Closing, please wait...\n");
            return NULL;
        }
    }
    return NULL;
}
#endif


/*
 * getchar_immediate():
 * If the appropriate conditions are set, immediately returns the first character in
 * the input buffer, or -1 if the buffer is empty.
 *
 * This function is designed for looping until the user presses enters input.
 * Instead of doing something complicated like spawning another thread to wait
 * for user input, just disable input blocking and call this function.
 *
 * On Windows, there is a function that does this, _kbhit(), so just call that.
 *
 * On Unix systems, you will have to manually disable blocking. To do this, call
 * fcntl() with the O_NONBLOCK flag. For this program's use case, you can enable
 * and disable blocking with enable_special_input() and disable_special_input().
 *
 * return: The character read from the input buffer or -1 if the input buffer is
 *      empty.
 */
char getchar_immediate() {
    #ifdef _WIN32
        if(_kbhit()) 
            return getchar();
        return -1;
    #else
        return getchar();
    #endif
}


/*
 * enable_special_input():
 * Configures user input for a "press-any-button-to-quit" mode.
 *
 * On Windows, there is a already function that does what we want, so nothing needs
 * to be changed.
 * 
 * On Unix, this involves two things:
 * 1) Setting non-blocking input for stdin so the main loop can call a 
 *          non-blocking version of getchar()
 * 2) Disabling the controlling terminal's "canon mode", which enters user
 *          input into the inpupt buffer as its entered instead of waiting
 *          for a newline char. This lets the user press any one key and have
 *          the program quit instead of having to press enter.
 */
void enable_special_input() {
    #ifndef _WIN32

        // Taken from:
        // https://gamedev.stackexchange.com/questions/146256/how-do-i-get-getchar-to-not-block-the-input
        
        
        int flags;
        if((flags = fcntl(0, F_GETFL, 0)) == -1)
            flags = 0;
        fcntl(0, F_SETFL, flags | O_NONBLOCK);

        struct termios chars;
        tcgetattr(0, &chars);
        chars.c_lflag &= ~ICANON;
        tcsetattr(0, TCSANOW, &chars);

    #endif
}


/*
 * disable_special_input():
 * Undoes the changes made in enable_special_input(), returns the terminal
 * back to normal input mode.
 *
 * Puts input in blocking mode and enable canon mode.
 */
void disable_special_input() {
    #ifndef _WIN32
        //Get current control flags
        int flags;
        if((flags = fcntl(0, F_GETFL, 0)) == -1)
            flags = 0;
        // Reenable blocking
        fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

        //Reenable canon mode
        struct termios chars;
        tcgetattr(0, &chars);
        chars.c_lflag |= ICANON;
        tcsetattr(0, TCSANOW, &chars);
    #endif
}




/******************
 * MISC FUNCTIONS *
 ******************/


/* 
 * usage():
 * Prints information about how to run the program
 */
void usage() {
    printf("***** Aural Landscapes ******\n");
    printf("-- Algorithmic Composition --\n\n");
    #ifdef USE_GRAPHICS
    printf("       (Graphics Mode)\n");
    #endif


    printf("Usage:\n");

    #ifdef _WIN32
        #ifdef USE_GRAPHICS
        printf("aural_landscapes.exe input.png -o output.png --hide-rect\n");
        #else
        printf("aural_landscapes.exe input.png -o output.png\n");
        #endif
    #else
        #ifdef USE_GRAPHICS
        printf("./aural_landscapes input.png -o output.png --hide-rect\n");
        #else
        printf("./aural_landscapes input.png -o output.png\n");
        #endif
    #endif

    printf("input.png:                  input file must be a png image\n");
    printf("-o output.wav (optional):   writes audio to the given filename\n");
    
    #ifdef USE_GRAPHICS
    printf("--hide-rect (optional):     hides the rectangle display on the image\n\n");
    #endif
}
