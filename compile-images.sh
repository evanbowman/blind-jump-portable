
python3 prep_spritesheet_for_gba.py

grit bgr_spritesheet.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv bgr_spritesheet.s source/graphics
mv bgr_spritesheet.h source/graphics

grit bgr_spritesheet_boss0.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv bgr_spritesheet_boss0.s source/graphics
mv bgr_spritesheet_boss0.h source/graphics

grit bgr_tilesheet.png -gB4 -Mw 4 -Mh 3 -gTFF00FF
mv bgr_tilesheet.s source/graphics
mv bgr_tilesheet.h source/graphics

grit bgr_overlay.png -gB4 -gTFF00FF
mv bgr_overlay.s source/graphics
mv bgr_overlay.h source/graphics

grit bgr_overlay_journal.png -gB4 -gTFF00FF
mv bgr_overlay_journal.s source/graphics
mv bgr_overlay_journal.h source/graphics
