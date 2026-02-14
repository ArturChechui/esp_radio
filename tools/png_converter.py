from PIL import Image
import sys

def png_to_ssd1306_bytes(path):
    img = Image.open(path).convert("1")  # 1-bit
    w, h = img.size
    if h % 8 != 0:
        raise ValueError(f"Height must be multiple of 8 for SSD1306 pages, got {h}")
    pages = h // 8
    px = img.load()

    out = []
    for page in range(pages):
        for x in range(w):
            b = 0
            for bit in range(8):
                y = page * 8 + bit
                # In PIL "1": 0 = black, 255 = white.
                if px[x, y] != 0:
                    b |= (1 << bit)
            out.append(b)
    return w, h, out

def print_c_array(w, h, data):
    print("{{")
    for i, v in enumerate(data):
        if i % 16 == 0:
            print("    ", end="")
        print(f"0x{v:02X}, ", end="")
        if i % 16 == 15:
            print()
    if len(data) % 16 != 0:
        print()
    print("}},")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python png_to_ssd1306.py input.png")
        sys.exit(1)
    path = sys.argv[1]
    w, h, data = png_to_ssd1306_bytes(path)
    print_c_array(w, h, data)
