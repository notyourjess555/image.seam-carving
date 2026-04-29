# Seam Carving - Content-Aware Image Resizing 	

## Project Overview
This project implemented C programming that shrinks images by removing "seams" (paths of low-importance pixels) while preserving perceptually important features with objects and shapes intact ratio. The algorithm calculates pixel importance using dual-gradient energy functions to identify what matters most in an image (edges, faces, text). Using dynamic programming and bottom-up DP, it finds the best seam path efficiently by removing pixels along minimum-energy seams (connected paths of pixels with the lowest visual importance).
The results are comparable to Adobe Photoshop and other Photoshop's content-aware resizing feature, preserving subject matter and aspect ratio during image reduction.

## What is Seam Craving?
Seam carving is a content-aware image resizing algorithm first published by Avidan & Shamir in 2007, now used in Adobe Photoshop. Unlike cropping (which cuts out content) or scaling (which distorts everything), seam carving removes vertical or horizontal "seams" — connected paths of pixels from top to bottom (or left to right) — where each seam is chosen to minimize the loss of noticeable image content.


## Key Implementation Details
1. Edge handling

Used modulo arithmetic to wrap around to the opposite side of the image, ensuring every pixel has valid neighbors for energy calculation

Edge handling (modulo wrap-around):
- int x_left = (j-1 + im->width) % im->width;
- int y_up = (i-1 + im->height) % im->height;

2. DP storage

Stored the cost array is stored as a flattened 1D array (i*width + j) instead of a 2D array, which is more memory-efficient and cache-friendly. 

Flattened 2D array indexing:
- (*best_arr)[i * width + j] = energy + min;

3. Energy storage

Used of energy/10 cast to uint8_t to compresses range for image display

4. Seam recovery

Backtracking compares top, top_left, top_right with boundary checks

5. Seam removal

Used continue to skips seam column and dest_col to tracks the new position


## Main functions
1. Energy Calculation (calc_energy)

This function computes dual-gradient energy for each pixel using RGB differences.
Handles image edges by wrapping around using modulo arithmetic.
Stores energy as grayscale (energy/10 cast to uint8_t).

The formula measures how much the color changes relative to neighboring pixels: 


Δ²x(y,x) = Rx² + Gx² + Bx²

Δ²y(y,x) = Ry² + Gy² + By²

Energy = √(Δ²x + Δ²y)

Where:
- Rx(y,x) = red(y, x+1) - red(y, x-1) (wrapping around at edges)
- High energy = edges, textures, important features
- Low energy = smooth gradients, less noticeable areas

2. Dynamic Programming (dynamic_seam)

Builds a flattened 1D array where each cell = pixel energy + min(above, above-left, above-right). Uses i*width + j indexing for memory efficiency.

3. Seam Recovery (recover_path)

Finds the smallest cost in the bottom row, then backtracks up by choosing the parent pixel with the smallest cumulative energy (checking top, top-left, top-right with bounds).

4. Seam Removal (remove_seam)

Creates a new image with width-1. Copies all pixels except the seam column, shifting remaining left using the dest_col counter.


## Why Dynamic Programming?
A naive approach would test every possible vertical path — O(3^H) which is impossible for large images. 
Dynamic Programming reduces this to O(H × W) by reusing previously computed costs.

## Extra informations with Technologies Used
Language: C 

Data structures: Flattened 1D arrays, custom RGB struct

Key concepts: Dynamic programming, modulo edge handling, grayscale conversion

# Results
## orginal image
<img width="516" height="332" alt="Screenshot 2026-04-28 at 8 26 45 PM" src="https://github.com/user-attachments/assets/144213e7-0383-4d96-88b1-edacedf91e1f" />


## final image
<img width="420" height="332" alt="Screenshot 2026-04-28 at 8 26 55 PM" src="https://github.com/user-attachments/assets/7f902dad-ddd1-45fe-983d-cdbf57125603" />

