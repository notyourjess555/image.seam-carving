#include "c_img.h"
#include "seamcarving.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_PIXELS 1000000ULL
#define MAX_DIMENSION 4096
#define MAX_REMOVAL 300

static int read_dimensions(const char *filename, size_t *height, size_t *width) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) return 0;

  unsigned char header[4];
  int ok = fread(header, 1, sizeof(header), fp) == sizeof(header);
  long file_length = -1;
  if (ok && fseek(fp, 0, SEEK_END) == 0) file_length = ftell(fp);
  fclose(fp);
  if (!ok || file_length < 4) return 0;

  *height = ((size_t)header[0] << 8) | header[1];
  *width = ((size_t)header[2] << 8) | header[3];

  if (*height == 0 || *width == 0 || *height > MAX_DIMENSION ||
      *width > MAX_DIMENSION || *height * *width > MAX_PIXELS) {
    return 0;
  }

  unsigned long long expected = 4ULL + 3ULL * *height * *width;
  return (unsigned long long)file_length == expected;
}

static int parse_target(const char *text, int *target) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < 1 || value > MAX_DIMENSION) {
    return 0;
  }
  *target = (int)value;
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: runner input.bin output.bin target-width\n");
    return 2;
  }

  size_t height = 0;
  size_t width = 0;
  int target_width = 0;
  if (!read_dimensions(argv[1], &height, &width) || !parse_target(argv[3], &target_width) ||
      (size_t)target_width >= width || width - (size_t)target_width > MAX_REMOVAL) {
    fprintf(stderr, "invalid image dimensions or target width\n");
    return 2;
  }

  struct rgb_img *current = NULL;
  read_in_img(&current, argv[1]);
  if (current == NULL || current->raster == NULL) {
    fprintf(stderr, "could not read image\n");
    return 3;
  }

  while (current->width > (size_t)target_width) {
    double *energy = NULL;
    double *best = NULL;
    int *path = NULL;
    struct rgb_img *next = NULL;

    calc_energy(current, &energy);
    if (energy == NULL) goto allocation_failure;
    dynamic_seam(energy, (int)current->height, (int)current->width, &best);
    if (best == NULL) {
      free(energy);
      goto allocation_failure;
    }
    recover_path(best, (int)current->height, (int)current->width, &path);
    if (path == NULL) {
      free(energy);
      free(best);
      goto allocation_failure;
    }
    remove_seam(current, &next, path);
    free(energy);
    free(best);
    free(path);
    if (next == NULL || next->raster == NULL) {
      if (next != NULL) destroy_image(next);
      destroy_image(current);
      fprintf(stderr, "could not allocate the resized image\n");
      return 3;
    }

    destroy_image(current);
    current = next;
  }

  write_img(current, argv[2]);
  destroy_image(current);

  struct stat output_stat;
  unsigned long long expected_output = 4ULL + 3ULL * height * (size_t)target_width;
  if (stat(argv[2], &output_stat) != 0 ||
      (unsigned long long)output_stat.st_size != expected_output) {
    fprintf(stderr, "could not write output image\n");
    return 4;
  }
  return 0;

allocation_failure:
  destroy_image(current);
  fprintf(stderr, "could not allocate seam data\n");
  return 3;
}
