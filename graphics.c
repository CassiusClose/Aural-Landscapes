#include "graphics.h"

#include <stdio.h>

/* Internal function declarations */
void clear_rect(Graphics* graphics);
void set_disp_pixel(Graphics* graphics, int x, int y, Uint32 rgba);
Uint32 get_orig_pixel(Graphics* graphics, int x, int y);



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
Graphics* create_graphics(char* title, int w, int h) {
    // Init SDl firsth
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("[SDL] Init error: %s\n", SDL_GetError());
        return NULL;
    }

    Graphics* graphics = (Graphics*) malloc(sizeof(Graphics));
    graphics->pixels = NULL;
    graphics->disp_pixels = NULL;

    // Init window
    graphics->window = SDL_CreateWindow(title, 100, 100, w, h, SDL_WINDOW_SHOWN);
    if(graphics->window == NULL) {
        printf("[SDL] Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        free(graphics);
        return NULL;
    }

    // Init renderer
    graphics->renderer = SDL_CreateRenderer(graphics->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(graphics->renderer == NULL) {
        SDL_DestroyWindow(graphics->window);
        printf("[SDL] Error creating renderer: %s\n", SDL_GetError());
        SDL_Quit();
        free(graphics);
        return NULL;
    }

    // Init texture
    graphics->texture = SDL_CreateTexture(graphics->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if(graphics->texture == NULL) {
        SDL_DestroyRenderer(graphics->renderer);
        SDL_DestroyWindow(graphics->window);
        printf("[SDL] Error creating texture: %s\n", SDL_GetError());
        SDL_Quit();
        free(graphics);
        return NULL;
    }

    graphics->width = w;
    graphics->height = h;

    return graphics;
}



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
void setPixels(Graphics* graphics, Uint32* pixels, int width, int height) {
    graphics->pixels = pixels;
    graphics->width = width;
    graphics->height = height;

    // If there were old display pixels, free them
    if(graphics->disp_pixels != NULL)
        free(graphics->disp_pixels);

    // Make copy of image in display pixels
    graphics->disp_pixels = (Uint32*) malloc(sizeof(Uint32)*width*height);
    memcpy(graphics->disp_pixels, pixels, sizeof(Uint32)*width*height);

    SDL_UpdateTexture(graphics->texture, NULL, graphics->disp_pixels, width*4);
}



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
void draw_rect(Graphics* graphics, int x, int y, int width, int height) {
    clear_rect(graphics);

    Uint32 color = 0xFFFFFFFF;

    // Set the borders of this rectangle to white
    for(int i = x; i < x+width; i++) {
        set_disp_pixel(graphics, i, y, color);
        set_disp_pixel(graphics, i, y+height, color);
    }
    for(int i = y; i < y+height; i++) {
        set_disp_pixel(graphics, x, i, color);
        set_disp_pixel(graphics, x+width, i, color);
    }

    graphics->rect_x = x;
    graphics->rect_y = y;
    graphics->rect_w = width;
    graphics->rect_h = height;
}


/*
 * clear_rect():
 * Removes the previously created rectangle overlay, setting those pixels
 * back to the original image's
 *
 * graphics:    A pointer to the Graphics struct to clear
 *
 */
void clear_rect(Graphics* graphics) {
    int x = graphics->rect_x;
    int y = graphics->rect_y;
    int w = graphics->rect_w;
    int h = graphics->rect_h;


    for(int i = x; i < x + w; i++) {
        set_disp_pixel(graphics, i, y, get_orig_pixel(graphics, i, y));
        set_disp_pixel(graphics, i, y+h, get_orig_pixel(graphics, i, y+h));
    }
    for(int i = y; i < y + h; i++) {
        set_disp_pixel(graphics, x, i, get_orig_pixel(graphics, x, i));
        set_disp_pixel(graphics, x+w, i, get_orig_pixel(graphics, x+w, i));
    }
}


/*
 * set_disp_pixel():
 * Sets the display pixel at the given location to the given value
 *
 * graphics:    A pointer to the Graphics struct to alter
 * x:           The x coordinate of the pixel
 * y:           The y coordinate of the pixel
 * rgba:        The color value to set the pixel to
 */
void set_disp_pixel(Graphics* graphics, int x, int y, Uint32 rgba) {
    graphics->disp_pixels[graphics->width*y + x] = rgba;
}

/*
 * get_orig_pixel():
 * Get the original image's pixel value at the given location, without any
 * overlays.
 *
 * graphics:    A pointer to the Graphics struct
 * x:           The x coordinate of the pixel
 * y:           The y coordinate of the pixel
 *
 * return :     The color value of the pixel
 */
Uint32 get_orig_pixel(Graphics* graphics, int x, int y) {
    return graphics->pixels[graphics->width*y + x];
}



/*
 * updateWindow():
 * Triggers a redraw of the graphics window with any new image changes.
 *
 * graphics:    A pointer to the Graphics struct to redraw
 */
void updateWindow(Graphics* graphics) {
    SDL_UpdateTexture(graphics->texture, NULL, graphics->disp_pixels, graphics->width*4);

    SDL_RenderClear(graphics->renderer);
    SDL_RenderCopy(graphics->renderer, graphics->texture, NULL, NULL);
    SDL_RenderPresent(graphics->renderer);
}



/*
 * isWindowOpen():
 * Returns whether the user has pressed the close on the SDL window.
 *
 * graphics:    A pointer to the Graphics object to poll
 *
 * return:      Boolean, whether the close button has been pressed.
 */
int isWindowOpen(Graphics* graphics) {
    SDL_Event event;
    SDL_PollEvent(&event);
    if(event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE)) {
        return 0;
    }
    return 1;
}



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
Uint32* convert_rgba_ints_to_Uint32(unsigned char* pixels, int length) {
    if(pixels == NULL)
        return NULL;

    // 4 RGBA chars to 1 RGBA Uint32
    if(length % 4 != 0) {
        printf("Couldn't convert rgba int pixels to Uint32: Must be a multiple of 4 ints\n");
        return NULL;
    }

    Uint32* newpix = (Uint32*) malloc(sizeof(Uint32) * length/4);

    for(int i = 0; i < length; i+= 4) {
        newpix[i/4] = pixels[i] | (pixels[i+1] << 8) | (pixels[i+2]) << 16 |
            (pixels[i+2] << 24);
    }

    return newpix;
}


/*
 * free_graphics():
 * Frees any resources associated with SDL and frees the saved pixel arrays.
 * Also frees the passed in pointer.
 *
 * graphics:    A pointer to the Graphics object to free
 *
 */
void free_graphics(Graphics* graphics) {
    SDL_DestroyTexture(graphics->texture);
    SDL_DestroyRenderer(graphics->renderer);
    SDL_DestroyWindow(graphics->window);
    SDL_Quit();
    free(graphics->pixels);
    free(graphics->disp_pixels);
    free(graphics);
}
