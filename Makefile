LINKER = -lportaudio -lsndfile -lm
OPTIONS = -Wall -o aural_landscapes -g
OPTIONS += $(USER_OPTIONS)
GRAPHICS = -lpthread -lSDL2main -lSDL2 -DUSE_GRAPHICS
CC = gcc

main: main.c oscillator.c audio_player.c breakpoints.c lodepng.c image.c key.c
	$(CC) $(OPTIONS) main.c oscillator.c audio_player.c breakpoints.c lodepng.c image.c key.c $(LINKER)

graphics: main.c oscillator.c audio_player.c breakpoints.c lodepng.c image.c key.c graphics.c
	$(CC) $(OPTIONS) main.c oscillator.c audio_player.c breakpoints.c lodepng.c image.c key.c graphics.c $(LINKER) $(GRAPHICS)

clean:
	rm run
