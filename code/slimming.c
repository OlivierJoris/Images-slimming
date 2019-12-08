/* ------------------------------------------------------------------------- *
 * Implementation of the slimming interface.
 * ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "slimming.h"

//Structure representing a table which will store the cost of each pixel.
typedef struct CostTable_t{
	size_t height, width; //Height and width of the image.
	float **table; //Table of size width * height that will store the cost of each pixel.
}CostTable;

//Differents color channels possible.
typedef enum{
    red,
    green,
    blue
}colorChannel;

/* ------------------------------------------------------------------------- *
 * Compute the cost of each pixel and stores it in a CostTable.
 *
 * PARAMETERS
 * image        the PNM image
 *
 * RETURN
 * >= 0, the pixel energy of the pixel (i,j).
 * -1, the image pointer equals NULL.
 * -2, the data pointer of the image equals NULL.
 * -3, the i index is bigger than the height of the image.
 * -4, the j index is bigger than the width of the image.
 * ------------------------------------------------------------------------- */
static CostTable* compute_cost_table(const PNMImage *image);

/* ------------------------------------------------------------------------- *
 * Free the memory of a CostTable.
 *
 * PARAMETERS
 * nCostTable    The CostTable we want to free.
 *
 * RETURN
 * /
 * ------------------------------------------------------------------------- */
static void destroy_cost_table(CostTable* nCostTable);

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
static float pixel_energy(const PNMImage *image, const size_t i, const size_t j);

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

/* ------------------------------------------------------------------------- *
 * Give the minimum between 2 values
 *
 * PARAMETERS
 * firstValue   the first value to compare
 * secondValue  the second value to compare
 *
 * RETURN
 * the minimum between firstValue and secondValue
 * ------------------------------------------------------------------------- */
static inline float min_with_two_arguments(const float firstValue, const float secondValue);

/* ------------------------------------------------------------------------- *
 * Give the minimum between 3 values
 *
 * PARAMETERS
 * firstValue   the first value to compare
 * secondValue  the second value to compare
 * thirdValue   the third value to compare
 *
 * RETURN
 * the minimum between firstValue, secondValue, and thirdValue.
 * ------------------------------------------------------------------------- */
static inline float min_with_three_arguments(const float firstValue, const float secondValue, const float thirdValue);

/* ------------------------------------------------------------------------- *
 * Give the minimum energy cost of the groove which stops at the pixel (i,j)
 *
 * PARAMETERS
 * image        the PNM image
 * i            the line index of the pixel
 * j            the column index of the pixel
 *
 * RETURN
 * >= 0, the minimum energy cost of the groove.
 * -1, the image pointer equals NULL.
 * -2, the data pointer of the image equals NULL.
 * -3, the i index is bigger than the height of the image.
 * -4, the j index is bigger than the width of the image.
 * ------------------------------------------------------------------------- */
static unsigned int min_cost_energy(const PNMImage *image, const size_t i, const size_t j);


static CostTable* compute_cost_table(const PNMImage *image){
	if(!image || !image->data){
		fprintf(stderr, "** ERROR : image is not a valid pointer in compute_cost_table.\n");
		return NULL;
	}

	//Allocate struct.
	CostTable* nCostTable = malloc(sizeof(CostTable));
	if(!nCostTable){
		fprintf(stderr, "** ERROR while allocating a CostTable in compute_cost_table.\n");
		return NULL;
	}

	nCostTable->width = image->width;
	nCostTable->height = image->height;

	//Allocate table attribut.
	nCostTable->table = malloc(sizeof(float*) * image->height);
	if(!nCostTable->table){
		fprintf(stderr, "** ERROR while allocating an array of float* in compute_cost_table.\n");
		if(nCostTable)
			free(nCostTable);
		return NULL;
	}

	for(size_t i = 0; i < image->height; ++i){

		nCostTable->table[i] = calloc(image->width, sizeof(float));
		if(!nCostTable->table[i]){
			fprintf(stderr, "** ERROR while allocating an array of float in compute_cost_table.\n");
			for(size_t j = 0; j < i; ++j){
				if(nCostTable->table[j])
					free(nCostTable->table[j]);
			}

			free(nCostTable->table);
			free(nCostTable);
			return NULL;
		}
	}//End for()

	//The first line will be filled with the energy of each pixel.
	for(size_t i = 0; i < image->width; ++i){
		nCostTable->table[0][i] = pixel_energy(image, 0, i);
		if(nCostTable->table[0][i] < 0){
			fprintf(stderr, "** ERROR while filling the first line of the CostTable in compute_cost_table.\n");
			destroy_cost_table(nCostTable);
			return NULL;
		}
	}

	//TOP-BOTTOM APPROACH to fill the CostTable.

	//Start at i = 1 because the first line is already initialized.
	//Initialized the other lines.
	for(size_t i = 1; i < image->height; ++i){

		//On the left edge of the image, only 2 possible values.
		nCostTable->table[i][0] = pixel_energy(image, i, 0) +
			min_with_two_arguments(nCostTable->table[i-1][0], nCostTable->table[i-1][1]);

		//In the middle of the image.
		for(size_t j = 1; j < image->width - 1; ++j){

			nCostTable->table[i][j] = pixel_energy(image, i, j) +
				min_with_three_arguments(nCostTable->table[i-1][j], nCostTable->table[i-1][j+1], nCostTable->table[i-1][j-1]);

		}//End for()

		//On the right edge of the image, only 2 possible values.
		nCostTable->table[i][image->width-1] = pixel_energy(image, i, image->width-1) +
			min_with_two_arguments(nCostTable->table[i-1][image->width-1], nCostTable->table[i-1][image->width-2]);

	}//End for()

	return nCostTable;
}//End compute_cost_table()

static void destroy_cost_table(CostTable* nCostTable){

	if(nCostTable){

		if(nCostTable->table){

			for(size_t i = 0; i < nCostTable->height; ++i){

				if(nCostTable->table[i])
					free(nCostTable->table[i]);
			}//End for()

			free(nCostTable->table);
		}

		free(nCostTable);
	}

	return;
}//End of destroy_cost_table()

static float pixel_energy(const PNMImage *image, const size_t i, const size_t j){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in pixel_energy.\n");
        return -1;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in pixel_energy.\n");
        return -2;
    }

    if(i >= image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in pixel_energy.\n");
        return -3;
    }

    if(j >= image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in pixel_energy.\n");
        return -4;
    }

    return color_energy(image, i, j, red) + color_energy(image, i, j, green) +
           color_energy(image, i, j, blue);
}//End pixel_energy()

static float color_energy(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in color_energy.\n");
        return -1.0;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in color_energy.\n");
        return -2.0;
    }

    if(i >= image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in color_energy.\n");
        return -3.0;
    }

    if(j >= image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in color_energy.\n");
        return -4.0;
    }

    //Extremes cases (pixel is on a edge of the image).


    if(i == image->height - 1 && j == image->width - 1){ //Bottom right corner.
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(i == 0 && j == 0){ //Top left corner.
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j == image->width - 1){ //Top right corner.
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(i == image->height - 1 && j == 0){ //Bottom left corner.
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j < image->width - 1 && j > 0){ //Top.
        return (abs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == image->height - 1 && j < image->width - 1 && j > 0){ //Bottom.
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == 0 && i < image->height - 1 && i > 0){ //Left.
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (abs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == image->width - 1 && i < image->height - 1 && i > 0){ //Right.
        return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    //If the pixel isn't on a egde of the image.
    return (abs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
           (abs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
}//End color_energy()

static int color_value(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in color_value.\n");
        return -1;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in color_value.\n");
        return -2;
    }

    if(i >= image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in color_value.\n");
        return -3;
    }

    if(j >= image->width){
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
}//End color_value()

static inline float min_with_two_arguments(const float firstValue, const float secondValue){
	if(firstValue < secondValue)
		return firstValue;
	else
		return secondValue;
}//End of min_with_two_arguments()

static inline float min_with_three_arguments(const float firstValue, const float secondValue, const float thirdValue){
    if(firstValue < secondValue && firstValue < thirdValue)
        return firstValue;

    if(secondValue < thirdValue)
        return secondValue;

    return thirdValue;
}//End min_with_three_arguments()

static unsigned int min_cost_energy(const PNMImage *image, const size_t i, const size_t j){
    if(!image){
        fprintf(stderr, "** ERROR : image is not a valid pointeur (= NULL) in min_cost_energy.\n");
        return -1;
    }

    if(!image->data){
        fprintf(stderr, "** ERROR : data is not a valid pointeur (= NULL) in min_cost_energy.\n");
        return -2;
    }

    if(i > image->height){
        fprintf(stderr, "** ERROR : i index is bigger than the height of the picture in min_cost_energy.\n");
        return -3;
    }

    if(j > image->width){
        fprintf(stderr, "** ERROR : j index is bigger than the width of the picture in min_cost_energy.\n");
        return -4;
    }

    if(i == 0)
        return pixel_energy(image, i, j);

    //Bottom-up approach
    return pixel_energy(image, i, j) + min_with_three_arguments(min_cost_energy(image, i - 1, j), min_cost_energy(image, i - 1, j + 1), min_cost_energy(image, i - 1, j - 1));
}//End min_cost_energy()

PNMImage* reduceImageWidth(const PNMImage* image, size_t k){
    //Test of the function pixel_energy()
    printf("Energy of the pixel (%lu, %lu) : %d.\n", k, k, (int)pixel_energy(image, k, k));

	//Test of the function min_with_two_arguments()
	printf("Min between 21 and 7 is : %f\n", min_with_two_arguments(21, 7));

    //Test of the function min_with_three_arguments()
    printf("Min between 17, 53 and %lu : %f.\n", k, min_with_three_arguments(17, 53, k));

    //Test of the function min_cost_energy()
    printf("The min cost of the groove which stops at the pixel (%d, %d) is composed of a energy of %u.\n", 4, 4, min_cost_energy(image, 4, 4));

	//Compute the CostTable.
	clock_t start = clock();
	CostTable* nCostTable = compute_cost_table(image);
	clock_t end = clock();
	if(nCostTable){
		printf("* CostTable was constructed without issues in ");
		printf("%lf seconds.\n", ((double) (end - start)) / CLOCKS_PER_SEC);
		printf("Cost of the pixel (%d, %d) is %f\n", 4, 4, nCostTable->table[4][4]);
	}else{
		printf("** ERROR while creating the CostTable.\n");
	}

	destroy_cost_table(nCostTable);

    return NULL;
}//End reduceImageWidth()
