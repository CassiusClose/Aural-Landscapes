<h2>Aural Landscapes</h2>
<h4>An algorithmic-composition program that processes image data to generate ambient music. </h4>

Cassius Close, cclose@u.rochester.edu

Final project for Audio Software Design I (AME262) at University of Rochester

<h3>Compiling</h3>

Aural Landscapes requires PortAudio and libsndfile to already be installed.
To compile, run\

    "make"

or run\

    "gcc -o aural_landscapes main.c oscillator.c audio_player.c breakpoints.c
    lodepng.c image.c key.c -lportaudio -lsndfile -lm"


If you want to see the image displayed on the screen, you'll need to install
SDL2. Then compile with\

    "make graphics"

or\

    "gcc -o aural_landscapes main.c oscillator.c audio_player.c breakpoints.c
    lodepng.c image.c key.c -lportaudio -lsndile -lm -lpthread -lSDL2main -lSDL2
    -DUSE_GRAPHICS"


You may need to include -Iinclude on Windows, I'm not sure.

I've included some example images in the resources/ folder. You can also play
around with the provided breakpoint and key files in that folder to change how the
notes sound and what keys are selected.
