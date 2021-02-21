from PIL import Image
import os
import sys
import platform
import struct


dir_path = os.path.dirname(os.path.realpath(__file__))


def convert_img(name):
    im = Image.open(dir_path + "/../../images/" + name + ".png")
    im = im.convert('RGB')

    with open(dir_path + "/" + name + '.pspimg', 'wb') as raw:
        for y in range(0, im.height):
            for x in range(0, im.width):
                r, g, b = im.getpixel((x, y))
                raw.write(struct.pack("=B", r))
                raw.write(struct.pack("=B", g))
                raw.write(struct.pack("=B", b))


if __name__ == "__main__":
    convert_img(sys.argv[1])
