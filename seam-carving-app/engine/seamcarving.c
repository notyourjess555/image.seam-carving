#include "c_img.h"
#include <stdio.h>
#include <math.h>

//new double array version, replace using greyscale, to keep image quality
void calc_energy(struct rgb_img *im, double **energy){

    if (energy == NULL) return;
    *energy = NULL;
    if (im == NULL || im->raster == NULL || im->height == 0 || im->width == 0 ||
        im->height > SIZE_MAX / im->width / sizeof(double)) return;

    int height = im->height;
    int width = im->width;

    *energy = malloc(height * width * sizeof(double));
    if (*energy == NULL) return;

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){

            int x_right = (j+1) % width;
            int x_left = (j-1+width) % width;
            int y_down = (i+1) % height;
            int y_up = (i-1+height) % height;

            int Rx = get_pixel(im, i, x_right, 0) - get_pixel(im, i, x_left, 0);
            int Gx = get_pixel(im, i, x_right, 1) - get_pixel(im, i, x_left, 1);
            int Bx = get_pixel(im, i, x_right, 2) - get_pixel(im, i, x_left, 2);

            int Ry = get_pixel(im, y_down, j, 0) - get_pixel(im, y_up, j, 0);
            int Gy = get_pixel(im, y_down, j, 1) - get_pixel(im, y_up, j, 1);
            int By = get_pixel(im, y_down, j, 2) - get_pixel(im, y_up, j, 2);

            int delta_x_sqr = Rx*Rx + Gx*Gx + Bx*Bx;
            int delta_y_sqr = Ry*Ry + Gy*Gy + By*By;

            double pixel_energy = sqrt(delta_x_sqr + delta_y_sqr);

            (*energy)[i * width + j] = pixel_energy;
        }
    }
}

//changed for double energy arry instead of greyscale
void dynamic_seam(double *energy, int height, int width, double **best_arr){

    if (best_arr == NULL) return;
    *best_arr = NULL;
    if (energy == NULL || height <= 0 || width <= 0 ||
        (size_t)height > SIZE_MAX / (size_t)width / sizeof(double)) return;

    *best_arr = (double *)malloc(width * height * sizeof(double));
    if (*best_arr == NULL) return;

    //fill the rest
    for(int i =0; i < height; i++){ //row

        for(int j =0; j < width; j++){ //col

            //get the energy
            double pixel_energy = energy[i * width + j];


            //if it is the first row
            if(i == 0){

                (*best_arr)[i * width + j] = pixel_energy; //same value
            }
            else{

                //say the min is the one actually on top of the current pixel
                double min = (*best_arr)[(i-1) * width + j] ;

                //consider the edges: check top left, if at col 0, not left
                if(j > 0){ //then have to consider left

                    double left = (*best_arr)[(i - 1)* width + (j-1)];
                    if (left < min){
                        min = left;
                    }

                }

                //check top right, if at col width-1, not left
                if(j < width -1){//then have to consider right

                    double right = (*best_arr)[(i-1) * width + (j+1)];
                    if (right < min){
                        min = right;
                    }
                }

                //store
                (*best_arr)[i * width + j] = pixel_energy +  min;


            }
        }
    }
}

void recover_path(double *best, int height, int width, int **path){

    if (path == NULL) return;
    *path = NULL;
    if (best == NULL || height <= 0 || width <= 0 || (size_t)height > SIZE_MAX / sizeof(int)) return;

    *path = (int*)malloc(sizeof(int) * height);
    if (*path == NULL) return;

    int min_col=0; //bug fixed Nov 1st 2026
    //search < min is never true, meaning min_col never gets assigned.

    double min = best[(height-1) * width]; //start with first col

    //find the min in the last row
    //note: use height-1 for indexing
    for(int j = 1; j < width; j++){

        //find the last row, min
        double search = best[(height-1) * width + j];
        if(search < min){
            min = search;
            min_col = j;
        }
    }

    //backtracking
    //logic: min(above_cost) = best[i][j] - energy[i][j]

    //already know the min path for last roe
    (*path)[height-1] = min_col;

    for(int i = height-1; i > 0 ; i--){

        //set up the values to check
        //now we at this row min cost (row * width + col)
        int current_col = (*path)[i];

        //check top, no edge case
        //trying to find: best[i-1][prev_col] + (something) = best[i][j]
        double top = best[(i-1) * width + current_col];

        //set up set the min first
        min_col = current_col;
        min = top;

        //check top left, if at col 0, no left, no need to check
        if(current_col > 0){
            double top_left = best[(i-1) * width + (current_col-1)];

            if(top_left < min){
                min_col = current_col - 1;
                min = top_left;
            }
        }//bug fixed: Nov 1st 2026
        //for C evaluates the array access before checking the if condition.
        //when current_col == 0, you're trying to access column -1.

        //check top right
        if(current_col < width-1){
            double top_right = best[(i-1) * width + (current_col+1)];

            if(top_right < min){
                min_col = current_col + 1;
                min = top_right;
            }
        }//bug fixed: Nov 1st 2026 same reason for top left

        (*path)[i-1] = min_col;

    }

}



void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path){

    if (dest == NULL) return;
    *dest = NULL;
    if (src == NULL || src->raster == NULL || src->width < 2 || path == NULL) return;

    //the image after compress
    create_img(dest, src->height, src->width-1);
    if (*dest == NULL) return;

    for(int i =0; i < src->height; i++){ //row

        //the column to remove per row
        int remove = path[i];
        int dest_col= 0;

        for(int j =0; j < src->width; j++){ //col

            if (j == remove){

                continue; //skip this column
            }

            //if not the skip col, get the pixels
            uint8_t red = get_pixel(src, i, j, 0);
            uint8_t green = get_pixel(src, i, j, 1);
            uint8_t blue = get_pixel(src, i, j, 2);

            //set the pixel of that dest location
            set_pixel(*dest, i, dest_col, red, green, blue);
            dest_col++;
        }
    }

}
