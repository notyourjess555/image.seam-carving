#include <stdio.h>
#include <stdlib.h>
#include "c_img.h"
#include "seamcarving.h"

int main(void) {  
    
    struct rgb_img *im;
    struct rgb_img *cur_im;
    double *energy;
    double *best;
    int *path;

    read_in_img(&im, "HJoceanSmall.bin");

    for(int i = 0; i < 5; i++){
        printf("i = %d\n", i);
        calc_energy(im, &energy);
        dynamic_seam(energy, im->height, im->width, &best);
        recover_path(best, im->height, im->width, &path);
        remove_seam(im, &cur_im, path);

        char filename[200];
        sprintf(filename, "img%d.bin", i);
        write_img(cur_im, filename);


        destroy_image(im);
        free(energy);
        free(best);
        free(path);
        im = cur_im;
    }
    destroy_image(im);

    return 0;
}