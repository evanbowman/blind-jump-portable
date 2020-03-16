
python3 prep_spritesheet_for_gba.py
grit bgr_spritesheet.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv bgr_spritesheet.s source/graphics
mv bgr_spritesheet.h source/graphics
