/* ------------------------------------------------------------------------- *
 * Implementation of the slimming interface.
 * ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>

#include "slimming.h"

//Differents color channels possible.
typedef enum{
    red,
    green,
    blue
}colorChannel;

/* ------------------------------------------------------------------------- *
 * Calculate the energy of the pixel (i, j).
 *
 * PARAMETERS
 * image        the PNM image
 * i            the line index of the pixel
 * j            the column index of the pixel
 *
 * RETURN
 * >= 0, the pixel energy of the pixel (i,j).
 * -1, the image pointer equals NULL.
 * -2, the data pointer of the image equals NULL.
 * -3, the i index is bigger than the height of the image.
 * -4, the j index is bigger than the width of the image.
 * ------------------------------------------------------------------------- */
static int pixel_energy(const PNMImage *image, const size_t i, const size_t j);

/* ------------------------------------------------------------------------- *
 * Calculate the energy of a channel color of the pixel (i, j)
 *
 * PARAMETERS
 * image        the PNM image
 * i            the line index of the pixel
 * j            the column index of the pixel
 * channel      the channel color of the pixel
 *
 * RETURN
 * >= 0, the pixel energy of the channel color "channel" of the pixel (i,j).
 * -1.0, the image pointer equals NULL.
 * -2.0, the data pointer of the image equals NULL.
 * -3.0, the i index is bigger than the height of the image.
 * -4.0, the j index is bigger than the width of the image.
 * ------------------------------------------------------------------------- */
static float color_energy(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel);

/* ------------------------------------------------------------------------- *
 * Give the value associated to a channel of the pixel (i, j)
 *
 * PARAMETERS
 * image        the PNM image
 * i            the line index of the pixel
 * j            the column index of the pixel
 * channel      the channel color of the pixel
 *
 * RETURN
 * >= 0, the value associated of the channel color "channel" of the pixel (i,j).
 * -1, the image pointer equals NULL.
 * -2, the data pointer of the image equals NULL.
 * -3, the i index is bigger than the height of the image.
 * -4, the j index is bigger than the width of the image.
 * -5, the channel doesn't exist.
 * ------------------------------------------------------------------------- */
static int color_value(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel);

static int pixel_energy(const PNMImage *image, const size_t i, const size_t j){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in pixel_energy.\n");
        return -1;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in pixel_energy.\n");
        return -2;
    }
    
    if(i > image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in pixel_energy.\n");
        return -3;
    }
    
    if(j > image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in pixel_energy.\n");
        return -4;
    }

    return color_energy(image, i, j, red) + color_energy(image, i, j, green) +  
           color_energy(image, i, j, blue);
}

static float color_energy(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in color_energy.\n");
        return -1.0;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in color_energy.\n");
        return -2.0;
    }
    
    if(i > image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in color_energy.\n");
        return -3.0;
    }
    
    if(j > image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in color_energy.\n");
        return -4.0;
    }
    
    //Extremes cases (pixel is on a edge of the image).
    if(j == image->width && i == image->height){
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) + 
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(i == 0 && j == 0){
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j == image->width){
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(j == 0 && i == image->height){
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) + 
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j != image->width && j != 0){
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == image->height && j != image->width && j != 0){
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) + 
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == 0 && i != image->height && i != 0){
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == image->width && i != image->height && i != 0){
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    //If the pixel isn't on a egde of the image.
    return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) + 
           (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
}

static int color_value(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in color_value.\n");
        return -1;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in color_value.\n");
        return -2;
    }
    
    if(i > image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in color_value.\n");
        return -3;
    }
    
    if(j > image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in color_value.\n");
        return -4;
    }

    switch(channel){
        case red:
            return image->data[(i * image->width) + j].red;
            break;
        case green:
            return image->data[(i * image->width) + j].green;
            break;
        case blue:
            return image->data[(i * image->width) + j].blue;
            break;
        default:
           fprintf(stderr, "** ERROR : color of the actual pixel is unknown in color_value.\n");
           return -5;
           break;
    }
}

PNMImage* reduceImageWidth(const PNMImage* image, size_t k){
    //Test of the function pixel_energy()
    printf("%d\n", pixel_energy(image, k, k));

    return NULL;
}
