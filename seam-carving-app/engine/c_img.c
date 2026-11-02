#include "c_img.h"

#include <math.h>
#include <stdio.h>

void create_img(struct rgb_img** im, size_t height, size_t width) {
  if (im == NULL) return;
  *im = NULL;
  if (height == 0 || width == 0 || height > SIZE_MAX / 3 / width) return;

  struct rgb_img* created = (struct rgb_img*)malloc(sizeof(struct rgb_img));
  if (created == NULL) return;
  created->raster = (uint8_t*)malloc(3 * height * width);
  if (created->raster == NULL) {
    free(created);
    return;
  }
  created->height = height;
  created->width = width;
  *im = created;
}

int read_2bytes(FILE* fp) {
  uint8_t bytes[2];
  if (fp == NULL || fread(bytes, 1, sizeof(bytes), fp) != sizeof(bytes)) return -1;
  return (((int)bytes[0]) << 8) + (int)bytes[1];
}

void write_2bytes(FILE* fp, int num) {
  uint8_t bytes[2];
  bytes[0] = (uint8_t)((num & 0XFFFF) >> 8);
  bytes[1] = (uint8_t)(num & 0XFF);
  fwrite(bytes, 1, 1, fp);
  fwrite(bytes + 1, 1, 1, fp);
}

void read_in_img(struct rgb_img** im, char* filename) {
  if (im == NULL) return;
  *im = NULL;
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) return;
  int header_height = read_2bytes(fp);
  int header_width = read_2bytes(fp);
  if (header_height <= 0 || header_width <= 0) {
    fclose(fp);
    return;
  }
  size_t height = (size_t)header_height;
  size_t width = (size_t)header_width;
  create_img(im, height, width);
  if (*im == NULL || fread((*im)->raster, 1, 3 * width * height, fp) != 3 * width * height) {
    destroy_image(*im);
    *im = NULL;
  }
  fclose(fp);
}

void write_img(struct rgb_img* im, char* filename) {
  if (im == NULL || im->raster == NULL || im->height == 0 || im->width == 0 ||
      im->height > 65535 || im->width > 65535 ||
      im->height > SIZE_MAX / 3 / im->width) return;
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) return;
  write_2bytes(fp, im->height);
  write_2bytes(fp, im->width);
  fwrite(im->raster, 1, im->height * im->width * 3, fp);
  fclose(fp);
}

uint8_t get_pixel(struct rgb_img* im, int y, int x, int col) {
  return im->raster[3 * (y * (im->width) + x) + col];
}

void set_pixel(struct rgb_img* im, int y, int x, int r, int g, int b) {
  im->raster[3 * (y * (im->width) + x) + 0] = r;
  im->raster[3 * (y * (im->width) + x) + 1] = g;
  im->raster[3 * (y * (im->width) + x) + 2] = b;
}

void destroy_image(struct rgb_img* im) {
  if (im == NULL) return;
  free(im->raster);
  free(im);
}

void print_grad(struct rgb_img* grad) {
  int height = grad->height;
  int width = grad->width;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      printf("%d\t", get_pixel(grad, i, j, 0));
    }
    printf("\n");
  }
}
