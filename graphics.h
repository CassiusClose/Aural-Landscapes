#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "SDL2/SDL.h"

/*
 * Graphics:
 * Contains all the SDL structs needed do display one image in a graphical
 * window. Also contains variables for drawing a rectangle on top of this
 * image.
 */
typedef struct graphics_container {
    /* SDL Variables */
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    // Holds the unmodified image pixels
    Uint32* pixels; 
    // Holds modifications to the original image. This is what's displayed.
    Uint32* disp_pixels; 

    // Width and height of the image/graphics window
    int width;
    int height;

    // Dimensions for the rectangle to overlay on the image
    int rect_x;
    int rect_y;
    int rect_w;
    int rect_h;
} Graphics;



/*
 * create_graphics():
 * Creates a malloc'ed Graphics struct with the specified settings and
 * initializes all the SDL objects. 
 *
 * When the user is done, they should call free_graphics() to free
 * all its resources.
 *
 * title:       The title of the graphics window
 * w:           The width of the graphics window
 * h:           The height of the graphics window
 *
 * return:      A pointer to a malloc'ed Graphics struct.
 */
Graphics* create_graphics(char* title, int w, int h);


/*
 * updateWindow():
 * Triggers a redraw of the graphics window with any new image changes.
 *
 * graphics:    A pointer to the Graphics struct to redraw
 */
void updateWindow(Graphics* graphics);


/*
 * add_rect():
 * Displays a rectangle overlayed on the image with the given dimensions.
 * Will clear any previously displayed rectangle.
 *
 * graphics:    A pointer to the graphics object to draw on
 * x:           The top left x coordinate of the rectangle
 * y:           The top left y coordinate of the rectangle
 * w:           The width of the rectangle
 * h:           The height of the rectangle
 *
 */
void draw_rect(Graphics* graphics, int x, int y, int width, int height);



/*
 * setPixels():
 * Sets the image of the given Graphics struct to the given pixels. Will
 * overwite any overlays on the previous image.
 *
 * The pixels are in a 1d array, so the width and height arguments specify
 * how to draw the image. The pixel array must have a length of width*height.
 *
 * graphics:    A pointer to the Graphics object to alter
 * pixels:      The pixels to save into the Graphics object
 * width:       The width of the pixel array
 * height:      The height of the pixel array
 */
void setPixels(Graphics* graphics, Uint32* pixels, int width, int height);



/*
 * isWindowOpen():
 * Returns whether the user has pressed the close on the SDL window.
 *
 * graphics:    A pointer to the Graphics object to poll
 *
 * return:      Boolean, whether the close button has been pressed.
 */
int isWindowOpen(Graphics* graphics);



/*
 * convert_rgba_ints_to_Uint32():
 * Converts image data from a LodePNG format to an SDL format.
 *
 * LodePNG format: each R, G, B, A value is a separate char in an array
 * SDL format: each pixel stores RGBA in one Uint32
 *
 * Because each Uint32 holds 4 char values, the length of the input array 
 * must be divisible by 4.
 *
 * Returns a malloc'ed array of Uint32s, must be freed unless set as
 * a Graphics struct's pixels. Then, free_graphics() will free it.
 *
 * pixels:      An array of the char-format pixels
 * length:      The length of the pixel array, must be divisible by 4
 *
 * return:      A pointer to a malloc'ed array of Uint32 pixel values
 *
 */
Uint32* convert_rgba_ints_to_Uint32(unsigned char* pixels, int length);



/*
 * free_graphics():
 * Frees any resources associated with SDL and frees the saved pixel arrays.
 * Also frees the passed in pointer.
 *
 * graphics:    A pointer to the Graphics object to free
 *
 */
void free_graphics(Graphics* graphics);




#endif
