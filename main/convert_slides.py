#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "pillow",
# ]
# ///

import sys
import re
import argparse
from pathlib import Path
from PIL import Image

def rle_encode_image(img):
    """
    Apply run-length encoding to image pixels.

    Args:
        img: PIL Image at 1024x768

    Returns:
        List of tuples (count, color) where count is 1-256 and color is 24-bit RGB
    """
    TARGET_WIDTH = 1024
    TARGET_HEIGHT = 768
    pixels = img.load()

    encoded = []
    prev_color = None
    count = 0

    for y in range(TARGET_HEIGHT):
        for x in range(TARGET_WIDTH):
            r, g, b = pixels[x, y]
            color = (r << 16) | (g << 8) | b

            if color == prev_color and count < 256:
                count += 1
            else:
                if prev_color is not None:
                    encoded.append((count, prev_color))
                prev_color = color
                count = 1

    # Don't forget the last run
    if prev_color is not None:
        encoded.append((count, prev_color))

    return encoded

def process_image(img_path: Path):
    """
    Process a single image: resize and letterbox to 1024x768.

    Args:
        img_path: Path to the input image

    Returns:
        PIL.Image: Processed image at 1024x768
    """
    TARGET_WIDTH = 1024
    TARGET_HEIGHT = 768

    # Load the image
    img = Image.open(img_path)

    # Convert to RGB if necessary
    if img.mode != 'RGB':
        img = img.convert('RGB')

    # Calculate scaling to fit within target dimensions while maintaining aspect ratio
    img_ratio = img.width / img.height
    target_ratio = TARGET_WIDTH / TARGET_HEIGHT

    if img_ratio > target_ratio:
        # Image is wider than target - fit to width
        new_width = TARGET_WIDTH
        new_height = int(TARGET_WIDTH / img_ratio)
    else:
        # Image is taller than target - fit to height
        new_height = TARGET_HEIGHT
        new_width = int(TARGET_HEIGHT * img_ratio)

    # Resize the image
    img_resized = img.resize((new_width, new_height), Image.Resampling.LANCZOS)

    # Create a new black image with target dimensions
    final_img = Image.new('RGB', (TARGET_WIDTH, TARGET_HEIGHT), (0, 0, 0))

    # Calculate position to paste the resized image (center it)
    x_offset = (TARGET_WIDTH - new_width) // 2
    y_offset = (TARGET_HEIGHT - new_height) // 2

    # Paste the resized image onto the black background
    final_img.paste(img_resized, (x_offset, y_offset))

    return final_img

def find_slide_images(base_path: str):
    """
    Find all slide images matching the pattern base-<number>.png

    Args:
        base_path: Path pattern like "slides-%d.png" or just "slides"

    Returns:
        List of tuples (slide_number, Path) sorted by slide number
    """
    path = Path(base_path)

    # Extract the base name and directory
    if "%d" in base_path:
        # User provided pattern like "slides-%d.png"
        base_name = base_path.replace("%d", "*").replace(".png", "")
        pattern = base_path.replace("%d", "*")
    else:
        # User provided base name like "slides" or path to a directory
        if path.is_dir():
            base_name = "*"
            pattern = str(path / "*-*.png")
        else:
            base_name = path.stem
            pattern = str(path.parent / f"{base_name}-*.png")

    # Find all matching files
    if "%d" in base_path:
        search_path = Path(pattern).parent
        glob_pattern = Path(pattern).name
    else:
        search_path = path.parent if not path.is_dir() else path
        glob_pattern = f"{base_name}-*.png" if not path.is_dir() else "*-*.png"

    matching_files = list(search_path.glob(glob_pattern))

    # Extract slide numbers and sort
    slides = []
    for file_path in matching_files:
        # Extract number from filename
        match = re.search(r'-(\d+)\.png$', file_path.name)
        if match:
            slide_num = int(match.group(1))
            slides.append((slide_num, file_path))

    slides.sort(key=lambda x: x[0])
    return slides

def convert_images_to_c_header(base_path: str, output_path: str = "slides.h"):
    """
    Convert multiple slide images to a C header with arrays.

    Args:
        base_path: Path pattern for slides (e.g., "slides-%d.png" or "slides")
        output_path: Path to the output header file (default: slides.h)
    """
    TARGET_WIDTH = 1024
    TARGET_HEIGHT = 768

    # Find all slide images
    slides = find_slide_images(base_path)

    if not slides:
        print(f"Error: No slide images found matching pattern")
        print(f"Expected files like: <name>-0.png, <name>-1.png, etc.")
        sys.exit(1)

    print(f"Found {len(slides)} slide(s):")
    for slide_num, slide_path in slides:
        print(f"  Slide {slide_num}: {slide_path.name}")

    # Process all images
    processed_images = []
    for slide_num, slide_path in slides:
        print(f"Processing slide {slide_num}...")
        img = process_image(slide_path)
        rle_data = rle_encode_image(img)
        processed_images.append((slide_num, rle_data))
        print(f"  Original: {TARGET_HEIGHT * TARGET_WIDTH} pixels")
        print(f"  Compressed: {len(rle_data)} runs ({len(rle_data) / (TARGET_HEIGHT * TARGET_WIDTH) * 100:.1f}%)")

    # Generate C header
    with open(output_path, 'w') as f:
        f.write("#ifndef SLIDES_H\n")
        f.write("#define SLIDES_H\n\n")
        f.write(f"// Slide data: {len(slides)} slides, 1024x768 pixels each, RLE-compressed 24-bit RGB\n")
        f.write("// Format: upper 8 bits = run length (1-256), lower 24 bits = RGB color\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define SLIDE_WIDTH {TARGET_WIDTH}\n")
        f.write(f"#define SLIDE_HEIGHT {TARGET_HEIGHT}\n")
        f.write(f"#define NUM_SLIDES {len(slides)}\n\n")

        # Write each slide as a separate array
        for slide_num, rle_data in processed_images:
            f.write(f"static const uint32_t slide_{slide_num}_data[{len(rle_data)}] = {{\n")

            for i, (count, color) in enumerate(rle_data):
                if i % 8 == 0:
                    f.write("    ")

                # Pack count in upper 8 bits, color in lower 24 bits
                packed = ((count - 1) << 24) | color  # Store count-1 so 1-256 fits in 0-255

                f.write(f"0x{packed:08X}")
                if i < len(rle_data) - 1:
                    f.write(", ")

                # Add newline every 8 values for readability
                if (i + 1) % 8 == 0 and i < len(rle_data) - 1:
                    f.write("\n")

            f.write("\n};\n\n")

        # Write metadata about array sizes
        f.write("static const uint32_t slide_sizes[NUM_SLIDES] = {\n")
        for i, (slide_num, rle_data) in enumerate(processed_images):
            f.write(f"    {len(rle_data)}")
            if i < len(processed_images) - 1:
                f.write(",")
            f.write(f"  // slide_{slide_num}\n")
        f.write("};\n\n")

        # Write array of pointers to all slides
        f.write("static const uint32_t* const slides[NUM_SLIDES] = {\n")
        for i, (slide_num, _) in enumerate(processed_images):
            f.write(f"    (const uint32_t*)slide_{slide_num}_data")
            if i < len(processed_images) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        f.write("#endif // SLIDES_H\n")

    # Get actual file size
    file_size = Path(output_path).stat().st_size

    print(f"\nHeader file written to: {output_path}")
    print(f"Total slides: {len(slides)}")
    print(f"Array size per slide: {TARGET_HEIGHT} x {TARGET_WIDTH} = {TARGET_HEIGHT * TARGET_WIDTH} pixels")
    print(f"File size: {file_size / (1024 * 1024):.2f} MB ({file_size:,} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: uv run --script main.py <base_path>")
        print("Examples:")
        print("  uv run --script main.py slides-%d.png")
        print("  uv run --script main.py slides")
        print("  uv run --script main.py /path/to/slides")
        print("\nExpected files: <name>-0.png, <name>-1.png, <name>-2.png, ...")
        sys.exit(1)

    base_path = sys.argv[1]
    convert_images_to_c_header(base_path)