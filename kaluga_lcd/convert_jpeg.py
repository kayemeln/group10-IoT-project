#!/usr/bin/env python3
"""
Convert JPEG image to raw RGB565 format for ESP32 LCD display
"""

from PIL import Image
import struct

def jpeg_to_rgb565(input_file, output_file, target_width=320, target_height=240):
    """Convert JPEG to RGB565 raw binary format"""
    
    # Open and resize image
    img = Image.open(input_file)
    print(f"Original image size: {img.size}")
    
    # Resize to target dimensions
    img = img.resize((target_width, target_height), Image.Resampling.LANCZOS)
    
    # Convert to RGB if not already
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    print(f"Resized to: {img.size}")
    
    # Convert to RGB565
    with open(output_file, 'wb') as f:
        for y in range(target_height):
            for x in range(target_width):
                r, g, b = img.getpixel((x, y))
                
                # Convert 8-bit RGB to 5-6-5 format
                r5 = (r >> 3) & 0x1F
                g6 = (g >> 2) & 0x3F  
                b5 = (b >> 3) & 0x1F
                
                # Pack into 16-bit value (big endian)
                rgb565 = (r5 << 11) | (g6 << 5) | b5
                
                # Write as little endian (ESP32 format)
                f.write(struct.pack('<H', rgb565))
    
    file_size = target_width * target_height * 2
    print(f"Created {output_file} ({file_size} bytes)")

if __name__ == "__main__":
    jpeg_to_rgb565("spiffs_image/face_detect.jpeg", "spiffs_image/face_detect.rgb565")