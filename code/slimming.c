/* ------------------------------------------------------------------------- *
 * Implementation of the slimming interface.
 * Maxime GOFFART (180521) et Olivier JORIS (182113).
 * ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>

#include "slimming.h"

/* ------------------------------------------------------------------------- *
 *
 * STRUCTURES
 *
 * ------------------------------------------------------------------------- */

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
	PixelCoordinates *path; //Array which contains the coordinates of each pixel in the groove.
	float cost; //The cost of the groove.
}Groove;

//Differents color channels possible.
typedef enum{
    red,
    green,
    blue
}colorChannel;

/* ------------------------------------------------------------------------- *
 *
 * PROTOTYPES OF STATIC FUNCTIONS
 *
 * ------------------------------------------------------------------------- */

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
 * 0, source was successfully copied into destination.
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
static float color_value(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel);

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
static PixelCoordinates find_optimal_pixel(const CostTable* nCostTable, const size_t currentLine, const size_t currentRow);

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
static Groove* find_optimal_groove(const CostTable* nCostTable);

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
static int shift_left(PNMImage* image, const size_t position);

/* ------------------------------------------------------------------------- *
 * Remove Groove 'nGroove' in PNMImage 'image'.
 *
 * PARAMETERS
 * image    The image in which we want to remove the Groove 'nGroove'.
 * nGroove  The Groove we want to remove from PNMImage 'image'.
 *
 * RETURN
 * 0, the Groove 'nGroove' was removed from PNMImage 'image'.
 * -1, error while shifting elements in data attribut of PNMImage.
 * -2, pointer to image equals NULL.
 * -3, pointer to data attribut of image equals NULL.
 * -4, pointer to nGroove equals NULL.
 * -5, pointer to path attribut in Groove equals NULL.
 * -6, image has a width equal to 0.
 * ------------------------------------------------------------------------- */
static int remove_groove_image(PNMImage *image, const Groove* nGroove);

/* ------------------------------------------------------------------------- *
 * Update the cost table after removing a Groove.
 *
 * PARAMETERS
 * image      The image in which we have removed the Groove 'nGroove'.
 * nCostTable The costTable we want to update.
 * nGroove    The Groove we have removed from the image.
 *
 * RETURN
 * nCostTable, the costTable updated.
 * NULL, in case of error
 * ------------------------------------------------------------------------- */
static CostTable* update_cost_table(const PNMImage* image, CostTable* nCostTable, const Groove* optimalGroove);

/* ------------------------------------------------------------------------- *
 *
 * IMPLEMENTATION
 *
 * ------------------------------------------------------------------------- */

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
	if(!image || !image->data)
		return NULL;

	//Allocate struct.
	CostTable* nCostTable = malloc(sizeof(CostTable));
	if(!nCostTable)
		return NULL;

	nCostTable->width = image->width;
	nCostTable->height = image->height;

	//Allocate table attribut.
	nCostTable->table = malloc(sizeof(float*) * image->height);
	if(!nCostTable->table){
		if(nCostTable)
			free(nCostTable);
		return NULL;
	}

	for(size_t i = 0; i < image->height; ++i){

		nCostTable->table[i] = calloc(image->width, sizeof(float));
		if(!nCostTable->table[i]){
			for(size_t j = 0; j < i; ++j){
				if(nCostTable->table[j])
					free(nCostTable->table[j]);
			}

			free(nCostTable->table);
			free(nCostTable);
			return NULL;
		}
	}//End for()

	//We compute the cost table
	//We fill the first line.
	for(size_t i = 0; i < image->width; ++i){
		nCostTable->table[0][i] = pixel_energy(image, 0, i);

		if(nCostTable->table[0][i] < 0){
			destroy_cost_table(nCostTable);
			return NULL;
		}
	}

	//We fill the other lines.
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
	}

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
    if(!image)
        return -1;

    if(!image->data)
        return -2;

    if(i >= image->height)
        return -3;

    if(j >= image->width)
        return -4;

    return color_energy(image, i, j, red) + color_energy(image, i, j, green) +
           color_energy(image, i, j, blue);
}//End pixel_energy()

static float color_energy(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image)
        return -1.0;

    if(!image->data)
        return -2.0;

    if(i >= image->height)
        return -3.0;

    if(j >= image->width)
        return -4.0;

    //Extremes cases (pixel is on a edge of the image).

    if(i == image->height - 1 && j == image->width - 1){ //Bottom right corner.
        return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(i == 0 && j == 0){ //Top left corner.
        return (fabs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (fabs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j == image->width - 1){ //Top right corner.
        return (fabs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    if(i == image->height - 1 && j == 0){ //Bottom left corner.
        return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (fabs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == 0 && j < image->width - 1 && j > 0){ //Top.
        return (fabs(color_value(image, i, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(i == image->height - 1 && j < image->width - 1 && j > 0){ //Bottom.
        return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i, j, channel)) / 2) +
               (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == 0 && i < image->height - 1 && i > 0){ //Left.
        return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (fabs(color_value(image, i, j, channel) - color_value(image, i, j + 1, channel)) / 2);
    }

    if(j == image->width - 1 && i < image->height - 1 && i > 0){ //Right.
        return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
               (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j, channel)) / 2);
    }

    //If the pixel isn't on a egde of the image.
    return (fabs(color_value(image, i - 1, j, channel) - color_value(image, i + 1, j, channel)) / 2) +
           (fabs(color_value(image, i, j - 1, channel) - color_value(image, i, j + 1, channel)) / 2);
}//End color_energy()

static float color_value(const PNMImage *image, const size_t i, const size_t j, const colorChannel channel){
    if(!image)
        return -1;

    if(!image->data)
        return -2;

    if(i >= image->height)
        return -3;

    if(j >= image->width)
        return -4;

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

static PixelCoordinates find_optimal_pixel(const CostTable* nCostTable, const size_t currentLine, const size_t currentRow){

	PixelCoordinates nvPixel;
	nvPixel.line = 0;
	nvPixel.column = 0;

	if(!nCostTable || !nCostTable->table)
		return nvPixel;

	if(currentLine <= 0)
		return nvPixel;

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

static Groove* find_optimal_groove(const CostTable* nCostTable){
	if(!nCostTable || !nCostTable->table)
		return NULL;

	//Allocating the structure.
	Groove* optimalGroove = malloc(sizeof(Groove));
	if(!optimalGroove)
		return NULL;

	optimalGroove->cost = 0;

	/*
	 The path of a groove will always have a length equals to the height of the image.
	 The height of the image is equal to he height of the CostTable.
	*/
	optimalGroove->path = malloc(sizeof(PixelCoordinates) * nCostTable->height);
	if(!optimalGroove->path){
		if(optimalGroove)
			free(optimalGroove);
		return NULL;
	}

	//We will work with a BOTTOM-UP approach in the CostTable.

	//First we need to find the pixel with the minimum cost on the last line of the CostTable.

	float minLastLine = FLT_MAX;
	int positionLastLine = -1;

	for(size_t i = 0; i < nCostTable->width; ++i){
		if(nCostTable->table[nCostTable->height - 1][i] < minLastLine){
			minLastLine = nCostTable->table[nCostTable->height - 1][i];
			positionLastLine = i;
		}
	}

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

static int shift_left(PNMImage* image, const size_t position){
	if(!image)
		return -1;
	if(!image->data)
		return -2;

	for(size_t i = position; i < (image->width * image->height) - 1; ++i)
		image->data[i] = image->data[i + 1];

	return 0;
}//End shift_left()

static int remove_groove_image(PNMImage *image, const Groove* nGroove){
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
		if(resultShift < 0)
			return -1;

	}

	return 0;
}//End remove_groove_image()

static CostTable* update_cost_table(const PNMImage* image, CostTable* nCostTable, const Groove* optimalGroove){
	if(!nCostTable)
		return NULL;
	if(!optimalGroove)
		return NULL;

	//We have to update the cost table.

	//We shift elements of one position left (beginning at the groove column) on each line of the table.
	for(size_t i = 0; i < nCostTable->height; ++i){
		for(size_t j = optimalGroove->path[i].column; j < nCostTable->width - 1; ++j){
			nCostTable->table[i][j] = nCostTable->table[i][j + 1];
		}
	}

	--nCostTable->width; //We reduced the table of one pixel on each line.

	size_t firstColumn = optimalGroove->path[0].column;

	//First line
	if(firstColumn >= image->width)
		nCostTable->table[0][firstColumn] = pixel_energy(image, 0, image->width - 1);
	else
		nCostTable->table[0][firstColumn] = pixel_energy(image, 0, firstColumn);

	int j = 0; //Must be int because it can be < 0 and size_t is an unsigned type.

	//We only update the changed values. Represent a cone beginning at the first pixel of the groove.
	for(int i = 1; i < (int)image->height; ++i){

		j = (int)firstColumn - i;
		if(j < 0)
			j = 0;

		for(; j < (int)image->width && j <= (int)firstColumn + i; ++j){

			//On the left edge of the image, only 2 possible values.
			if(j == 0){
				nCostTable->table[i][j] = pixel_energy(image, i, j) +
					min_with_two_arguments(nCostTable->table[i-1][j], nCostTable->table[i-1][j+1]);

				if(nCostTable->table[i][j] < 0){
					destroy_cost_table(nCostTable);
					return NULL;
				}
			}

			if(j != 0 && j != (int)image->width - 1){
				//In the middle of the image.
				nCostTable->table[i][j] = pixel_energy(image, i, j) +
					min_with_three_arguments(nCostTable->table[i-1][j], nCostTable->table[i-1][j+1], nCostTable->table[i-1][j-1]);

				if(nCostTable->table[i][j] < 0){
					destroy_cost_table(nCostTable);
					return NULL;
				}
			}

			//On the right edge of the image, only 2 possible values.
			if(j == (int)image->width - 1){
				nCostTable->table[i][j] = pixel_energy(image, i, j) +
					min_with_two_arguments(nCostTable->table[i-1][j], nCostTable->table[i-1][j-1]);

				if(nCostTable->table[i][j] < 0){
					destroy_cost_table(nCostTable);
					return NULL;
				}
			}
		}//End for()
	}

	return nCostTable;
}//End update_cost_table()

PNMImage* reduceImageWidth(const PNMImage* image, size_t k){

	//Create the PNMImage which will contain the image with a width of image->width - 'k'.
	PNMImage* reducedImage = createPNM(image->width, image->height);
	if(!reducedImage)
		return NULL;

	//Copy 'image' into 'reducedImage'.
	int resultCopy = copy_pnm_image(image, reducedImage);
	if(resultCopy < 0)
		return NULL;

	Groove* optimalGroove;

	//Compute the the CostTable. Dynamic programming - memoization.
	CostTable* nCostTable = compute_cost_table(reducedImage);
	if(!nCostTable){
		freePNM(reducedImage);
		return NULL;
	}

	for(size_t number = 0; number < k; ++number){

		optimalGroove = find_optimal_groove(nCostTable);

		int resultRemove = remove_groove_image(reducedImage, optimalGroove);
		if(resultRemove < 0){
			destroy_groove(optimalGroove);
			destroy_cost_table(nCostTable);
			freePNM(reducedImage);
			return NULL;
		}

		nCostTable = update_cost_table(reducedImage, nCostTable, optimalGroove);
		if(nCostTable == NULL){
			destroy_groove(optimalGroove);
			destroy_cost_table(nCostTable);
			freePNM(reducedImage);
			return NULL;
		}

		destroy_groove(optimalGroove);
		optimalGroove = NULL;

	}//Fin for()

	if(optimalGroove)
		destroy_groove(optimalGroove);
	if(nCostTable)
		destroy_cost_table(nCostTable);

    return reducedImage;
}//End reduceImageWidth()
