#include "image.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define BYTEDEPTH 1 // Number of bytes per value (r, g, b, a)
#define BYTESPP (BYTEDEPTH*4) // Number of bytes per pixel


/* Internal function declarations */
void set_pixel(Image* image, int x, int y, int r, int g, int b, int a);
Pixel* get_pixel(Image* image, int x, int y);


/*
 * load_imagefile():
 * Loads a given .png image file to raw pixel data in char format. Sets the
 * image's width and height in the specified pointers. Returns a malloc'ed
 * struct of pixel data, where each char is an R, G, B, or A value of the
 * pixels in order. 
 *
 * The user will need to free the array when done with it.
 *
 * filename:    The image file to load (must be .png)
 * w:           Pointer to an int in which the image's width will be stored
 * h:           Pointer to an int in which the image's height will be stored
 *
 * return:      A malloc'ed array of pixel data, size w*h*4
 */
unsigned char* load_imagefile(char* filename, unsigned int* w, unsigned int* h) {
    unsigned char* pixels = 0;
    if(lodepng_decode_file(&pixels, w, h,
            filename, LCT_RGBA, BYTEDEPTH*8) != 0) {
        return NULL;
    }

    return pixels;
}

/*
 * load_to_image():
 * Converts an array of char pixel data to an Image struct.
 *
 * Note: both load_image() and create_image() are taken.
 *
 * Returns a malloc'ed image struct that must be freed with free_image()
 *
 * rawpix:      The array of char pixel data
 * width:       The width of the data
 * height:      The height of the data
 *
 */
Image* load_to_image(unsigned char* rawpix, int width, int height) {
    Image* image = (Image*) malloc(sizeof(Image));
    image->width = width;
    image->height = height;
    image->pixels = (Pixel*) malloc(sizeof(Pixel)*width*height);

    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            set_pixel(image, x, y,
                    rawpix[BYTESPP*y*width + BYTESPP*x],
                    rawpix[BYTESPP*y*width + BYTESPP*x + BYTEDEPTH],
                    rawpix[BYTESPP*y*width + BYTESPP*x + BYTEDEPTH*2],
                    rawpix[BYTESPP*y*width + BYTESPP*x + BYTEDEPTH*3]);
        }
    }

    return image;
}



/*
 * avg_warmth():
 * Returns the average warmth of a region as an integer between -255 and 255.
 *
 * image:       A pointer to the Image to analyze
 * x:           The top left x coordinate of the region
 * y:           The top left y coordinate of the region
 * w:           The width of the region
 * h:           The height of the region
 *
 * return:      The average warmth between -255 and 255
 */
int avg_warmth(Image* image, int x, int y, int w, int h) {
    int sum = 0;
    Pixel* p;
    for(int i = x; i < x+w; i++) {
        for(int j = y; j < y+h; j++) {
            p = get_pixel(image, i, j);
            //Warmth is just red - blue
            sum += (p->r - p->b);
        }
    }

    return sum/(float)(w*h);
}


/*
 * tot_avg_warmth():
 * Returns the average warmth of the entire image as an integer between -255
 * and 255.
 *
 * image:       A pointer to the Image to analyze
 *
 * return:      The average warmth between -255 and 255
 */
int tot_avg_warmth(Image* image) {
    return avg_warmth(image, 0, 0, image->width, image->height);
}


/*
 * perc_brightness():
 * Returns the brightness of the given pixel as a float between 0 and 1.
 *
 * image:       A pointer to the Image to analyze
 * x:           The x coordinate of the pixel
 * y:           The y coordinate of the pixel
 *
 * return:      The brightness between 0 and 1
 *
 */
float perc_brightness(Image* image, int x, int y) {
    Pixel* p = get_pixel(image, x, y);
    // Taken from http://alienryderflex.com/hsp.html
    float val = sqrt(0.299*p->r*p->r + 0.587*p->g*p->g + 0.114*p->b*p->b);
    float max = sqrt(0.299*255*255 + 0.587*255*255 + 0.114*255*255);
    return val/max; 
}

/*
 * avg_perc_brightness():
 * Returns the average bright of a region of pixel as a float between 0 and 1.
 *
 * image:       A pointer to the Image to analyze
 * x:           The top left x coordinate of the region
 * y:           The top left y coordinate of the region
 * w:           The width of the region
 * h:           The height of the region
 *
 * return:      The average brightness between 0 and 1
 *
 */
float avg_perc_brightness(Image* image, int x, int y, int w, int h) {
    float sum = 0;
    for(int i = x; i < x+w; i++) {
        for(int j = y; j < y+h; j++) {
            Pixel* p = get_pixel(image, i, j);
            sum += sqrt(0.299*p->r*p->r + 0.587*p->g*p->g + 0.114*p->b*p->b);
        }
    }
    float max = sqrt(0.299*255*255 + 0.587*255*255 + 0.114*255*255);

    return sum/(float)(w*h*max);
}




/* 
 * set_pixel():
 * Sets the pixel at the given location to the given values.
 *
 * image:       A pointer to the Image struct to set
 * x:           The x coordinate of the pixel
 * y:           The y coordinate of the pixel
 * r:           The red value of the pixel
 * g:           The green value of the pixel
 * b:           The blue value of the pixel
 * a:           The alpha value of the pixel
 */
void set_pixel(Image* image, int x, int y, int r, int g, int b, int a) {
    image->pixels[y*image->width + x].r = r;
    image->pixels[y*image->width + x].g = g;
    image->pixels[y*image->width + x].b = b;
    image->pixels[y*image->width + x].a = a;
}


/*
 * get_pixel():
 * Returns a pointer to the Pixel struct for the given location on the image.
 *
 * image:       A pointer to the Image to look at
 * x:           The x coordinate of the pixel
 * y:           The y coordinate of the pixel
 *
 * return:      A pointer to the Pixel struct at that location
 */
Pixel* get_pixel(Image* image, int x, int y) {
    return image->pixels + y*image->width + x; 
}



void free_image(Image* image) {
    free(image->pixels);
    free(image);
}

