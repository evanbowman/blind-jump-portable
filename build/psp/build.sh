
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

python3 $DIR/psp_images.py

bin2c $DIR/test.pspimg $DIR/spritesheet.c spritesheet
bin2c $DIR/test_tilemap.pspimg $DIR/test_tilemap.c test_tilemap
bin2c $DIR/test_overlay.pspimg $DIR/overlay.c overlay_img
bin2c $DIR/tilesheet_top.pspimg $DIR/tilesheet_top.c tilesheet_top_img
bin2c $DIR/charset.pspimg $DIR/charset.c charset_img

make -C $DIR -j8
