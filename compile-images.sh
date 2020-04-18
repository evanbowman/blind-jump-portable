
python3 prep_spritesheet_for_gba.py

grit tmp/spritesheet.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv spritesheet.s source/graphics
mv spritesheet.h source/graphics

grit tmp/spritesheet2.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv spritesheet2.s source/graphics
mv spritesheet2.h source/graphics

grit tmp/spritesheet_boss0.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
mv spritesheet_boss0.s source/graphics
mv spritesheet_boss0.h source/graphics

grit tmp/tilesheet.png -gB4 -Mw 4 -Mh 3 -gTFF00FF
mv tilesheet.s source/graphics
mv tilesheet.h source/graphics

grit tmp/tilesheet2.png -gB4 -Mw 4 -Mh 3 -gTFF00FF
mv tilesheet2.s source/graphics
mv tilesheet2.h source/graphics

grit tmp/overlay.png -gB4 -gTFF00FF
mv overlay.s source/graphics
mv overlay.h source/graphics

grit tmp/overlay_journal.png -gB4 -gTFF00FF
mv overlay_journal.s source/graphics
mv overlay_journal.h source/graphics

grit tmp/old_poster_flattened.png -gB4 -gTFF00FF
mv old_poster_flattened.s source/graphics
mv old_poster_flattened.h source/graphics
