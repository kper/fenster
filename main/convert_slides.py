#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "pillow",
# ]
# ///

import sys
from pathlib import Path
from PIL import Image

def convert_image_to_c_array(image_path: str, output_path: str = "slides.h"):
    """
    Convert an image to 1024x768 with letterboxing and generate a C header.

    Args:
        image_path: Path to the input image
        output_path: Path to the output header file (default: slides.h)
    """
    TARGET_WIDTH = 1024
    TARGET_HEIGHT = 768

    # Load the image
    img = Image.open(image_path)
    print(f"Loaded image: {img.size[0]}x{img.size[1]}")

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

    print(f"Final image: {TARGET_WIDTH}x{TARGET_HEIGHT}")

    # Convert to pixel array
    pixels = final_img.load()

    # Generate C header
    with open(output_path, 'w') as f:
        f.write("#ifndef SLIDES_H\n")
        f.write("#define SLIDES_H\n\n")
        f.write("// Image data: 1024x768 pixels, 32-bit RGBA format\n")
        f.write("// Generated from: {}\n".format(Path(image_path).name))
        f.write("#include <stdint.h>\n\n")
        f.write(f"static const uint32_t image_data[{TARGET_HEIGHT}][{TARGET_WIDTH}] = {{\n")

        for y in range(TARGET_HEIGHT):
            f.write("    {")
            for x in range(TARGET_WIDTH):
                r, g, b = pixels[x, y]
                # Create 32-bit color in ARGB format (0xAARRGGBB)
                color = 0xFF000000 | (r << 16) | (g << 8) | b

                f.write(f"0x{color:08X}")
                if x < TARGET_WIDTH - 1:
                    f.write(", ")

            f.write("}")
            if y < TARGET_HEIGHT - 1:
                f.write(",")
            f.write("\n")

        f.write("};\n\n")
        f.write("#endif // SLIDES_H\n")

    print(f"Header file written to: {output_path}")
    print(f"Array size: {TARGET_HEIGHT} x {TARGET_WIDTH} = {TARGET_HEIGHT * TARGET_WIDTH} pixels")
    print(f"File size: ~{(TARGET_HEIGHT * TARGET_WIDTH * 4) / (1024 * 1024):.1f} MB")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: uv run --script main.py <image_path>")
        sys.exit(1)

    image_path = sys.argv[1]

    if not Path(image_path).exists():
        print(f"Error: Image file '{image_path}' not found")
        sys.exit(1)

    convert_image_to_c_array(image_path)