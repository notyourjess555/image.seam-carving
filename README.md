# Seam Carving for Content-Aware Image Resizing 

A resizing application website that reduces image width while attempting to preserve visually important details.
on https://imageseamcrave.onrender.com

## What is Seam Craving?
Seam carving is a content-aware image resizing algorithm first published by Avidan & Shamir in 2007, now used in Adobe Photoshop. Unlike cropping (which cuts out content) or scaling (which distorts everything), seam carving removes vertical or horizontal "seams" — connected paths of pixels from top to bottom (or left to right) — where each seam is chosen to minimize the loss of noticeable image content.

## Project Overview
This project implemented C programming that shrinks images by removing "seams" (paths of low-importance pixels) while preserving perceptually important features with objects and shapes intact ratio. The algorithm calculates pixel importance using dual-gradient energy functions to identify what matters most in an image (edges, faces, text). Using dynamic programming and bottom-up DP, it finds the best seam path efficiently by removing pixels along minimum-energy seams (connected paths of pixels with the lowest visual importance).Unlike traditional resizing methods that scale the entire image, seam carving removes paths of low-energy pixels to change an image's dimensions while minimizing visual distortion.


## Demo
<img width="579" height="503" alt="Screenshot 2026-11-01 at 10 14 14 PM" src="https://github.com/user-attachments/assets/c03ed6b9-0630-4517-ab85-6efee3d7849d" />


## How It Works

The resizing algorithm identifies and removes the least visually significant vertical seam in an image.

The process consists of four main steps:

1. **Energy Calculation:** Calculates the visual energy of each pixel using differences in neighbouring RGB values. Higher energy generally indicates stronger visual detail or edges.
2. **Dynamic Programming:** Builds a cumulative energy map to find the lowest-cost path from the top of the image to the bottom.
3. **Seam Recovery:** Traces backwards through the cumulative energy map to identify the minimum-energy vertical seam.
4. **Seam Removal:** Removes the selected seam and constructs an image that is one pixel narrower.

This process repeats until the image reaches the requested width.

## Technical Implementation

The project uses C for the core image-processing algorithm and Python with Flask for the web.

The core algorithm is implemented in C to provide direct control over image processing and memory management.

| Component | Responsibility |
|---|---|
| C | Energy calculation, dynamic programming, seam recovery and removal |
| Python | Backend processing and application logic |
| Flask | Handles communication between the frontend and backend |
| HTML, CSS, JavaScript | User interface and image interaction |

## Why Dynamic Programming?
A naive approach would test every possible vertical path — O(3^H) which is impossible for large images. 
Dynamic Programming reduces this to O(H × W) by reusing previously computed costs.


## Features

- Upload an image for resizing.
- Specify a target image width.
- Resize images using minimum-energy seam removal.
- Compare the original and resized images.
- Download the resulting image.

## Getting Started

### Requirements

- Python 3
- GCC (or another compatible C compiler)
- Required Python packages listed in `requirements.txt`

### Installation

Clone the repository:

```bash
git clone https://github.com/notyourjess555/SEAM-CARVING-REPOSITORY.git
cd SEAM-CARVING-REPOSITORY
```

Create and activate a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install dependencies:

```bash
pip install -r requirements.txt
```

### Run the Application

```bash
[ADD VERIFIED APPLICATION START COMMAND]
```

Open the local address printed by Flask in your browser.


##Main Functions

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

## C Algorithm Key Implementation Details
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

## Limitations

The current implementation uses pixel-based energy calculations to identify low-energy seams. It does not understand objects or semantic importance.

As a result, important subjects may become distorted if parts of them are included in a low-energy seam. Results also depend on the visual content of the input image.

The current implementation supports vertical seam removal, reducing image width while keeping its height unchanged.

## Future Improvements Plans

- Subject protection using manually selected protection region. (eg. person, objects)
- Horizontal seam removal for height adjustment.
- Improved seam selection and image-quality preservation.
- Performance optimization for larger images.

---

**Built by Jess Chan**

[GitHub](https://github.com/notyourjess555)
