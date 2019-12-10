/* ------------------------------------------------------------------------- *
 * Implementation of the slimming interface.
 * ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <stdbool.h>

#include "slimming.h"

//Structure representing a table which will store the cost of each pixel.
typedef struct CostTable_t{
	size_t height, width; //Height and width of the image.
	float **table; //Table of size width * height that will store the cost of each pixel.
}CostTable;

//Structure representing the coordinates of a pixel.
typedef struct PixelCoordinates_t{
	size_t line; //Line index of the pixel.
	size_t column; //Column index of the pixel.
}PixelCoordinates;

//Structure representing a groove.
typedef struct Groove_t{
	PixelCoordinates* path; //Array which contains the coordinates of each pixel in the groove.
	float cost; //The cost of the groove.
}Groove;

//Differents color channels possible.
typedef enum{
    red,
    green,
    blue
}colorChannel;

/* ------------------------------------------------------------------------- *
 * Copy the 'source' PNMImage into the 'destination' PNMImage.
 *
 * WARNING :
 * source and destination must have the same size !
 *
 * PARAMETERS
 * source         the PNM image which act as the source
 * destination    the PNM image which act as the destination
 *
 * RETURN
 * 0, source was copied successfully in destination.
 * -1, pointer to 'source' equals NULL.
 * -2, pointer to data attribut  of 'source' equals NULL.
 * -3, pointer to 'destination' equals NULL.
 * -4, pointer to data attribut of 'destination' equals NULL.
 * -5, 'source' and 'destination' don't have the same size.
 *
 * ------------------------------------------------------------------------- */
static int copy_pnm_image(const PNMImage *source, const PNMImage *destination);

/* ------------------------------------------------------------------------- *
 * Compute the cost of each pixel and stores it in a CostTable.
 *
 * PARAMETERS
 * image        the PNM image
 *
 * NOTE
 * The returned pointer should be freed using destroy_cost_table() after usage.
 *
 * RETURN
 * nCostTable, pointer to the CostTable associated to the 'image'.
 * NULL in case of error.
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
//static unsigned int min_cost_energy(const PNMImage *image, const size_t i, const size_t j);

/* ------------------------------------------------------------------------- *
 * Based on the pixel (currentLine, currentRow), find the pixel on the
 * line currentLine - 1 that has the smallest cost and which is a neighbour
 * of the pixel (currentLine, currentRow).
 *
 * PARAMETERS
 * nCostTable     the CostTable.
 * currentLine    the line index of the current pixel.
 * currentRow     the column index of the current pixel.
 *
 * RETURN
 * nvPixel        the pixel with the smallest cost and which is a neighbour
 *                of the pixel (currentLine, currentRow).
 * ------------------------------------------------------------------------- */
static PixelCoordinates find_optimal_pixel(CostTable* nCostTable, size_t currentLine, size_t currentRow);

/* ------------------------------------------------------------------------- *
 * Find the groove with the smallest energy (cost).
 *
 * PARAMETERS
 * nCostTable  The CostTable which contains the cost of each pixel.
 *
 * NOTE
 * The returned pointer should be freed using destroy_groove() after usage.
 *
 * RETURN
 * optimalGroove, pointer to the groove with the smallest energy (cost).
 * NULL in case of error.
 * ------------------------------------------------------------------------- */
static Groove* find_optimal_groove(CostTable* nCostTable);

/* ------------------------------------------------------------------------- *
 * Free the memory of a groove.
 *
 * PARAMETERS
 * nGroove  The Groove we want to free.
 *
 * RETURN
 * /
 * ------------------------------------------------------------------------- */
static void destroy_groove(Groove* nGroove);

/* ------------------------------------------------------------------------- *
 * Function to shift elements one position left from position to the end of
 * the data attribut of image.
 *
 * PARAMETERS
 * image     The image in which we need to shift elements.
 * position  The index of the beginning of the shift.
 *
 * RETURN
 * 0, shift has been realized.
 * -1, pointer to image equals NULL.
 * -2, pointer to data attribut of image equals NULL.
 * ------------------------------------------------------------------------- */
static int shift_left(PNMImage* image, size_t position);

/* ------------------------------------------------------------------------- *
 * Remove Groove 'nGroove' in PNMImage 'image'.
 *
 * PARAMETERS
 * image    The image in which we want to remove the Groove 'nGroove'.
 * nGroove  The Groove we want to remove in PNMImage 'image'.
 *
 * RETURN
 * 0, the Groove 'nGroove' was removed from PNMImage 'image'.
 * -1, error while shifting elements in data attribut of PNMImage.
 * -2, pointer to image equals NULL.
 * -3, pointer to data attribut of image equals NULL.
 * -4, pointer to groove equals NULL.
 * -5, pointer to path attribut in Groove equals NULL.
 * -6, image has a width equal to 0.
 * ------------------------------------------------------------------------- */
static int remove_groove_image(PNMImage *image, Groove* nGroove);

/* ------------------------------------------------------------------------- *
 * Update the cost table after having delete a groove.
 *
 * PARAMETERS
 * image      The image in which we have removed the Groove 'nGroove'.
 * nCostTable The costTable we want to update.
 * nGroove    The Groove we have deleted.
 *
 * RETURN
 * nCostTable, the costTable updated.
 * NULL, in case of error
 * ------------------------------------------------------------------------- */
static CostTable* update_cost_table(PNMImage* image, CostTable* nCostTable, Groove* optimalGroove);

static int copy_pnm_image(const PNMImage *source, const PNMImage *destination){
	if(!source)
		return -1;
	if(!source->data)
		return -2;
	if(!destination)
		return -3;
	if(!destination->data)
		return -4;

	//Check if 'source' and 'destination' have the same size.
	if(source->width != destination->width || source->height != destination->height)
		return -5;

	for(size_t i = 0; i < source->width * source->height; ++i)
		destination->data[i] = source->data[i];

	return 0;
}//End copy_pnm_image()

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

// static bool check_cost_table(CostTable* nCostTable, Groove* nGroove){
// 	if(!nCostTable)
// 		return -1;
// 	if(!nCostTable->table)
// 		return -2;

// 	if(!nGroove)
// 		return -3;
// 	if(!nGroove->path)
// 		return -4;

// 	/*
// 	 For each line in the CostTable, verify that the element on the left and on the right
// 	 of each element in the groove is equal to the element of the grove.
// 	 If so, the CostTable don't need to be rebuild. Elements just need to be shifted.
// 	 If not, the CostTable need to be rebuild.
// 	*/

// 	size_t columnPixelGroove;
// 	bool needToBeRebuild = false;

// 	for(size_t i = 0; i < nCostTable->height && !needToBeRebuild; ++i){

// 		columnPixelGroove = nGroove->path[i].column;

// 		if(columnPixelGroove == 0){ //The pixel of the groove is on the left edge of the image.
// 			//Compare the cost of the pixel of the groove and the one on his right.
// 			printf("Rentre dans if == 0.\n");
// 			if(nCostTable->table[i][columnPixelGroove] != nCostTable->table[i][columnPixelGroove + 1])
// 				needToBeRebuild = true;
// 		}

// 		if(columnPixelGroove == nCostTable->width - 1){ //The pixel of the groove is on the right edge of the image.
// 			//Compare the cost of the pixel of the groove and the one on his left.
// 			printf("Rentre dans if == width - 1\n");
// 			if(nCostTable->table[i][columnPixelGroove] != nCostTable->table[i][columnPixelGroove - 1])
// 				needToBeRebuild = true;
// 		}

// 		if(columnPixelGroove != 0 && columnPixelGroove != nCostTable->width - 1){
// 			//Compare the cost of the pixel of the groove and the one on his left and on his right.
// 			printf("Rentre dans if != 0 && != width - 1\n");
// 			if((nCostTable->table[i][columnPixelGroove - 1] != nCostTable->table[i][columnPixelGroove]) ||
// 			   (nCostTable->table[i][columnPixelGroove] != nCostTable->table[i][columnPixelGroove + 1]))
// 				needToBeRebuild = true;
// 		}
// 	}//End for()

// 	if(needToBeRebuild)
// 		return needToBeRebuild;

// 	// For each line in nCostTable, we should shift one position left every elements
// 	// from the position given by nGroov up to the end of the line.


// 	size_t beginningIndexShift;

// 	for(size_t lineIndex = 0; lineIndex < nCostTable->height; ++lineIndex){

// 		beginningIndexShift = nGroove->path[lineIndex].column;

// 		//Shifting
// 		for(size_t i = beginningIndexShift; i < nCostTable->width - 1; ++i)
// 			nCostTable->table[lineIndex][i] = nCostTable->table[lineIndex][i + 1];

// 	}//End for()

// 	return false;

// }//End check_cost_table()

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

/* EXHAUSTIVE METHOD
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
*/

static PixelCoordinates find_optimal_pixel(CostTable* nCostTable, size_t currentLine, size_t currentRow){

	PixelCoordinates nvPixel;
	nvPixel.line = 0;
	nvPixel.column = 0;

	if(!nCostTable || !nCostTable->table){
		fprintf(stderr, "** ERROR : pointer(s) to the CostTable in find_optimal_pixel equal(s) NULL.\n");
		return nvPixel;
	}

	if(currentLine <= 0){
		fprintf(stderr, "** ERROR : currentRow <= 0 in find_optimal_pixel().\n");
		return nvPixel;
	}

	nvPixel.line = currentLine - 1;

	/*
	 Based on the pixel (currentLine, currentRow), find the pixel on the line currentLine - 1
	 that has the minimal cost and which is a neighbour of the pixel (currentLine, currentRow).
	*/

	//If the pixel (currentLine, currentRow) is on the left edge of the image.
	if(currentRow == 0){
		if(nCostTable->table[currentLine - 1][currentRow] < nCostTable->table[currentLine - 1][currentRow + 1])
			nvPixel.column = currentRow;
		else
			nvPixel.column = currentRow + 1;

		return nvPixel;
	}

	//If the pixel (currentLine, currentRow) is on the right edge of the image.
	if(currentRow == nCostTable->width - 1){
		if(nCostTable->table[currentLine - 1][currentRow] < nCostTable->table[currentLine - 1][currentRow - 1])
			nvPixel.column = currentRow;
		else
			nvPixel.column = currentRow - 1;

		return nvPixel;
	}

	//If the pixel (currentLine, currentRow) isn't on the left egde nor on the right edge.

	if(nCostTable->table[currentLine - 1][currentRow - 1] < nCostTable->table[currentLine - 1][currentRow]
	   && nCostTable->table[currentLine - 1][currentRow] < nCostTable->table[currentLine - 1][currentRow + 1]){

		nvPixel.column = currentRow - 1;
		return nvPixel;
	}

	if(nCostTable->table[currentLine - 1][currentRow] < nCostTable->table[currentLine - 1][currentRow + 1]){
		nvPixel.column = currentRow;
		return nvPixel;
	}

	nvPixel.column = currentRow + 1;
	return nvPixel;

}//End find_optimal_pixel()

static Groove* find_optimal_groove(CostTable* nCostTable){
	if(!nCostTable || !nCostTable->table){
		fprintf(stderr, "** ERROR : pointer(s) to the CostTable in find_optimal_groove equal(s) NULL.\n");
		return NULL;
	}

	//Allocating the structure.
	Groove* optimalGroove = malloc(sizeof(Groove));
	if(!optimalGroove){
		fprintf(stderr, "** ERROR while allocating memory for the structure in find_optimal_groove().\n");
		return NULL;
	}

	optimalGroove->cost = 0;

	/*
	 The path of a groove will always have a length equals to the height of the image.
	 The height of the image is equal to he height of the CostTable.
	*/
	optimalGroove->path = malloc(sizeof(PixelCoordinates) * nCostTable->height);
	if(!optimalGroove->path){
		fprintf(stderr, "** ERROR while allocating memory for the path of a Groove in find_optimal_groove.\n");
		if(optimalGroove)
			free(optimalGroove);
		return NULL;
	}

	//We will work with a BOTTOM-UP approach in the CostTable.

	//First we need to find the pixel with the minimum cost on the last line of the CostTable.

	float minLastLine = FLT_MAX;
	int positionLastLine = -1;

	for(size_t i = 0; i < nCostTable->width; ++i){
		//printf("%f\n", nCostTable->table[nCostTable->height - 1][i]);
		if(nCostTable->table[nCostTable->height - 1][i] < minLastLine){
			minLastLine = nCostTable->table[nCostTable->height - 1][i];
			positionLastLine = i;
		}
	}

	//printf("The minimum in the last line is on position %d with a total cost of %f\n", positionLastLine, minLastLine);

	//We need to add that pixel in the path of the Groove.
	optimalGroove->path[nCostTable->height - 1].line = nCostTable->height - 1;
	optimalGroove->path[nCostTable->height - 1].column = positionLastLine;

	int currentLine = nCostTable->height - 2;

	PixelCoordinates nvPixel;

	while(currentLine >= 0){

		nvPixel = find_optimal_pixel(nCostTable, currentLine + 1, optimalGroove->path[currentLine + 1].column);

		optimalGroove->path[currentLine].line = currentLine;
		optimalGroove->path[currentLine].column = nvPixel.column;

		currentLine--;
	}//End while()

	optimalGroove->cost = minLastLine;
	//nCostTable->table[nCostTable->height - 1][positionLastLine] = FLT_MAX; //So we don't take twice the same groove.

	//for(size_t i = 0; i < nCostTable->height; ++i){
	//	printf("%lu || %lu\n", optimalGroove->path[i].line, optimalGroove->path[i].column);
	//}

	return optimalGroove;

}//End find_optimal_groove()

static void destroy_groove(Groove* nGroove){

	if(nGroove){

		if(nGroove->path)
			free(nGroove->path);

		free(nGroove);
	}

	return;
}//End destroy_groove()

static int shift_left(PNMImage* image, size_t position){
	if(!image)
		return -1;
	if(!image->data)
		return -2;

	for(size_t i = position; i < (image->width * image->height) - 1; ++i)
		image->data[i] = image->data[i + 1];

	return 0;
}//End shift_left()

static int remove_groove_image(PNMImage *image, Groove* nGroove){
	if(!image)
		return -2;
	if(!image->data)
		return -3;
	if(!nGroove)
		return -4;
	if(!nGroove->path)
		return -5;

	if(image->width <= 0)
		return -6;

	//We are going to remove a pixel on a each line. So we reduce the width of one pixel.
	image->width--;

	size_t indexOfPixelToRemove;
	int resultShift;

	//We are going to remove each pixel contained in 'nGroove'.
	for(size_t i = 0; i < image->height; ++i){

		indexOfPixelToRemove = (nGroove->path[i].line * image->width) + nGroove->path[i].column;
		resultShift = shift_left(image, indexOfPixelToRemove);
		if(resultShift < 0){
			fprintf(stderr, "** ERROR while shifting elements in remove_groove_image.\n");
			return -1;
		}

	}

	return 0;
}//End remove_groove_image()

static void save_cost_table(CostTable* nCostTable, char* filename){
	if(!nCostTable)
		return;
	if(!nCostTable->table)
		return;
	if(!filename)
		return;

	FILE* saveFile = fopen(filename, "w");
	if(!saveFile)
	 	return;

	for(size_t i = 0; i < nCostTable->height; ++i){
		for(size_t j = 0; j < nCostTable->width; ++j){
			fprintf(saveFile, "%d ", (int)nCostTable->table[i][j]);
		}
		fprintf(saveFile, "\n");
	}
	if(saveFile)
		fclose(saveFile);
}//End of save_cost_table()

static CostTable* update_cost_table(PNMImage* image, CostTable* nCostTable, Groove* optimalGroove){
	if(!nCostTable)
		return NULL;
	if(!optimalGroove)
		return NULL;
	
	//We have to update the cost table
	for(size_t i = 0; i < nCostTable->height; ++i){
		for(size_t j = optimalGroove->path[i].column; j < nCostTable->width - 1; ++j){
			nCostTable->table[i][j] = nCostTable->table[i][j + 1];
		}
	}
	
	--nCostTable->width;

	//We only update the changed values 
	for(size_t i = optimalGroove->path[0].column; i < image->width; ++i){
		nCostTable->table[0][i] = pixel_energy(image, 0, i);
		if(nCostTable->table[0][i] < 0){
			fprintf(stderr, "** ERROR while filling the first line of the CostTable in rebuild_cost_table.\n");
			destroy_cost_table(nCostTable);
			return NULL;
		}
	}
	
	for(size_t i = 1; i < image->height; ++i){

		//On the left edge of the image, only 2 possible values.
		nCostTable->table[i][0] = pixel_energy(image, i, 0) +
			min_with_two_arguments(nCostTable->table[i-1][0], nCostTable->table[i-1][1]);

		//In the middle of the image.
		for(size_t j = optimalGroove->path[i].column; j < image->width - 1; ++j){

			nCostTable->table[i][j] = pixel_energy(image, i, j) +
				min_with_three_arguments(nCostTable->table[i-1][j], nCostTable->table[i-1][j+1], nCostTable->table[i-1][j-1]);
		}//End for()

		//On the right edge of the image, only 2 possible values.
		nCostTable->table[i][image->width-1] = pixel_energy(image, i, image->width-1) +
			min_with_two_arguments(nCostTable->table[i-1][image->width-1], nCostTable->table[i-1][image->width-2]);

	}

	return nCostTable;
}

PNMImage* reduceImageWidth(const PNMImage* image, size_t k){

	printf("Picture size : %lux%lu\n", image->width, image->height);

	//Create the PNMImage which will contain the image with a width of image->width - 'k'.
	PNMImage* reducedImage = createPNM(image->width, image->height);
	if(!reducedImage){
		fprintf(stderr, "** ERROR while creating a PNMImage in reduceImageWidth.\n");
		return NULL;
	}

	//Copy 'image' into 'reducedImage'.
	int resultCopy = copy_pnm_image(image, reducedImage);
	if(resultCopy < 0){
		fprintf(stderr, "** ERROR while copying a PNMImage into another in reduceImageWidth.\n");
		return NULL;
	}else{
		printf("* image was successfully copied in reducedImage\n");
	}

	Groove* optimalGroove;
	//Compute the the CostTable. Dynamic programming - memoization.
	CostTable* nCostTable = compute_cost_table(reducedImage);
	if(!nCostTable){
		fprintf(stderr, "** ERROR while creating the cost table.\n");
		freePNM(reducedImage);
		return NULL;
	}

	// bool costTableNeedToBeRebuild;

	// size_t numberOfCostTables = 1;

	for(size_t number = 0; number < k; ++number){

		optimalGroove = find_optimal_groove(nCostTable);

		int resultRemove = remove_groove_image(reducedImage, optimalGroove);
		if(resultRemove < 0){
			fprintf(stderr, "** ERROR while removing groove in image\n");
			destroy_groove(optimalGroove);
			destroy_cost_table(nCostTable);
			freePNM(reducedImage);
			return NULL;
		}

		// costTableNeedToBeRebuild = check_cost_table(nCostTable, optimalGroove);
		// if(costTableNeedToBeRebuild){
		// 	destroy_cost_table(nCostTable);
		// 	nCostTable = compute_cost_table(reducedImage);
		// 	if(!nCostTable){
		// 		fprintf(stderr, "** ERROR while creating the cost table.\n");
		// 		destroy_groove(optimalGroove);
		// 		freePNM(reducedImage);
		// 		return NULL;
		// 	}
		// 	numberOfCostTables++;
		// }
		nCostTable = update_cost_table(reducedImage, nCostTable, optimalGroove);
		if(nCostTable == NULL){
			fprintf(stderr, "** ERROR while rebuilding the cost table.\n");
			destroy_groove(optimalGroove);
			destroy_cost_table(nCostTable);
			freePNM(reducedImage);
			return NULL;
		}

		destroy_groove(optimalGroove);
		optimalGroove = NULL;

	}//Fin for()

	// printf("Number of cost tables = %lu\n", numberOfCostTables);

	if(optimalGroove)
		destroy_groove(optimalGroove);
	if(nCostTable)
		destroy_cost_table(nCostTable);

    return reducedImage;
}//End reduceImageWidth()
