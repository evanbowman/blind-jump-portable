
# The Gameboy Advance console uses colors in a BGR format, but all my favorite
# graphics tools use RGBA.

from PIL import Image

im = Image.open('spritesheet.png')

r, g, b, a = im.split()

im = Image.merge('RGBA', (b, g, r, a))

im.save('bgr_spritesheet.png')


# Now the spritesheet just needs to be converted to a tileset. Use:
# grit bgr_spritesheet.png -gB4 -Mw 4 -Mh 4 -gTFF00FF
# And then move the output files into the source/ directory.
