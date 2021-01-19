#ifndef image_h
#define image_h

#include "lodepng.h"

/* 
 * Pixel:
 * Holds the red, blue, green, alpha values of one Pixel.
 */
typedef struct pixel {
    int r;
    int g;
    int b;
    int a;
} Pixel;


/*
 * Image:
 * Holds the Pixel data of an image and its dimensions
 */
typedef struct image {
    Pixel* pixels; 
    int width;
    int height;
} Image;


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
unsigned char* load_imagefile(char* filename, unsigned int* w, unsigned int* h);


/*
 * load_to_image():
 * Converts an array of char pixel data to an Image struct.
 *
 * Returns a malloc'ed image struct that must be freed with free_image()
 *
 * rawpix:      The array of char pixel data
 * width:       The width of the data
 * height:      The height of the data
 *
 */
Image* load_to_image(unsigned char* rawpix, int width, int height);


/*
 * free_image():
 * Frees any resources attached to the given Image struct.
 *
 * image:       A pointer to the Image struct to free.
 */
void free_image(Image* image);




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
float perc_brightness(Image* image, int x, int y);

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
float avg_perc_brightness(Image* image, int x, int y, int w, int h);

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
int avg_warmth(Image* image, int x, int y, int w, int h);


/*
 * tot_avg_warmth():
 * Returns the average warmth of the entire image as an integer between -255
 * and 255.
 *
 * image:       A pointer to the Image to analyze
 *
 * return:      The average warmth between -255 and 255
 */
int tot_avg_warmth(Image* image);


#endif
