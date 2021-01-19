AuralLandscapes
An algorithmic-composition project that processes image data

Cassius Close, cclose@u.rochester.edu


AuralLandscapes requires PortAudio and libsndfile to already be installed.
To compile, run 
    "make"
or run
    "gcc -o aural_landscapes main.c oscillator.c audio_player.c breakpoints.c
    lodepng.c image.c key.c -lportaudio -lsndfile -lm"


If you want to see the image displayed on the screen, you'll need to install
SDL2. Then compile with
    "make graphics"
or
    "gcc -o aural_landscapes main.c oscillator.c audio_player.c breakpoints.c
    lodepng.c image.c key.c -lportaudio -lsndile -lm -lpthread -lSDL2main -lSDL2
    -DUSE_GRAPHIS"



You may need to include -Iinclude on Windows, I'm not sure.


I've included some example images in the resources/ folder. You can also play
around with the provided breakpoints, etc. in that folder to change how the
notes sound.
