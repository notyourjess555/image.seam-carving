#include "c_img.h"
#include <stdio.h>
#include <math.h>

void calc_energy(struct rgb_img *im, struct rgb_img **grad){
    //create img
    create_img(grad, im->height, im->width);

    //use for loop to brighten or darkern the image
    for(int i =0; i < im->height; i++){

        for(int j =0; j < im->width; j++){

            //get pixel for each
            int x_right = (j+1) % im->width;
            int x_left = (j-1 + im->width) % im->width;
            int y_down = (i+1) % im->height;
            int y_up = (i-1 + im->height) % im->height;
            
            //for x
            int Rx = get_pixel(im, i, x_right, 0) - get_pixel(im, i, x_left, 0);
            //R(y, x+1) - R(y, x-1);
            int Gx = get_pixel(im, i, x_right, 1) - get_pixel(im, i, x_left, 1);
            //G(y, x+1) - G(y, x-1);
            int Bx = get_pixel(im, i, x_right, 2) - get_pixel(im, i, x_left, 2);
            //B(y, x+1) - B(y, x-1);

            //for y
            int Ry = get_pixel(im, y_down, j, 0) - get_pixel(im, y_up, j, 0);
            //R(y, x+1) - R(y, x-1);
            int Gy = get_pixel(im, y_down, j, 1) - get_pixel(im, y_up, j, 1);
            //G(y, x+1) - G(y, x-1);
            int By = get_pixel(im, y_down, j, 2) - get_pixel(im, y_up, j, 2);
            
            //the delta
            int delta_x_sqr = Rx * Rx + Gx * Gx + Bx * Bx;
            int delta_y_sqr = Ry * Ry + Gy * Gy + By * By;

            //energy
            int energy = sqrt(delta_x_sqr + delta_y_sqr);

            //uint8_t and divid 10
            uint8_t energy_value = (uint8_t) (energy/10.0);

            //set pixel to grad 
            //for each pixel, set the r, g, and b channels to the same value

            set_pixel(*grad, i,j, energy_value, energy_value, energy_value);


        }
    }
    
}

void dynamic_seam(struct rgb_img *grad, double **best_arr){

    int height = grad->height;
    int width = grad->width;

    *best_arr = (double *)malloc(width * height * sizeof(double));

    //fill the rest
    for(int i =0; i < grad->height; i++){ //row

        for(int j =0; j < grad->width; j++){ //col

            //get the energy
            double energy = get_pixel(grad, i, j, 0);


            //if it is the first row
            if(i == 0){

                (*best_arr)[i * width + j] = energy; //same value
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
                (*best_arr)[i * width + j] = energy +  min;


            }     
        }  
    }
}

void recover_path(double *best, int height, int width, int **path){

    *path = (int*)malloc(sizeof(int) * height);
    int min_col;
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
        double top_left =  best[(i-1) * width + (current_col-1)];
        if(top_left < min && current_col > 0){

            min_col = current_col -1;
            min = top_left;

        }

        //check top left, if at col width-1, not right, no need to check
        double top_right= best[(i-1) * width + (current_col+1)];
        if(top_right < min && current_col < width-1){

            min_col = current_col+1;
            min = top_right;

        }

        (*path)[i-1] = min_col;
       
    }

}



void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path){

    //the image after compress
    create_img(dest, src->height, src->width-1);

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