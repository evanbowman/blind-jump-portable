
python3 prep_spritesheet_for_gba.py

grit tmp/spritesheet.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
grit tmp/spritesheet2.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
grit tmp/spritesheet_boss0.png -gB4 -Mw 2 -Mh 4 -gTFF00FF
grit tmp/tilesheet.png -gB4 -Mw 4 -Mh 3 -gTFF00FF
grit tmp/tilesheet2.png -gB4 -Mw 4 -Mh 3 -gTFF00FF
grit tmp/overlay.png -gB4 -gTFF00FF
grit tmp/overlay_journal.png -gB4 -gTFF00FF
grit tmp/old_poster_flattened.png -gB4 -gTFF00FF

cat *.h >> images.h

mv images.h source/graphics
mv *.s source/graphics
