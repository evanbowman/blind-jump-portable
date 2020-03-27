
# The Gameboy Advance console uses colors in a BGR format, but all my favorite
# graphics tools use RGBA.

from PIL import Image


def rgb_to_bgr(file_name):
    """
    Convert rgb image file to bgr
    """
    im = Image.open(file_name)

    r, g, b, a = im.split()
    im = Image.merge('RGBA', (b, g, r, a))

    im.save('bgr_' + file_name)


for name in ['spritesheet.png', 'tilesheet.png', 'overlay.png', 'overlay_journal.png']:
    rgb_to_bgr(name)



# Now the spritesheet just needs to be converted to a tileset. Use:
# grit bgr_spritesheet.png -gB4 -Mw 4 -Mh 4 -gTFF00FF
# And then move the output files into the source/ directory.
