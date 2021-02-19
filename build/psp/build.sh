
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

python3 $DIR/psp_images.py

bin2c $DIR/test.pspimg $DIR/spritesheet.c spritesheet
bin2c $DIR/test_tilemap.pspimg $DIR/test_tilemap.c test_tilemap

make -C $DIR -j8
