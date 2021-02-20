#
# TODO: This is really bad. I don't know how to design a good image build
# pipeline around makefiles. Ideally, I would use CMake, and automatically
# rebuild only the changed images (as I'm already doing for Desktop and
# GBA). But CMake support for PSP builds is kinda terrible right now, so until I
# build the toolchain files myself, I'm stuck with this image build script.
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

INCLUDES=""
STUB="$(cat image_stub_begin.h)"
OBJS=""

for f in $DIR/../../images/*.png
do
    name=$(basename $f .png)
    echo "building ${DIR}/${name}.pspimg"
    python3 $DIR/psp_images.py $name
    bin2c $DIR/$name.pspimg $DIR/$name.img.c $name"_img"

    sed -i 's/static/const/' $DIR/$name.img.c

    rm $DIR/$name.pspimg

    INCLUDES="${INCLUDES}extern const unsigned int size_${name}_img;\n"
    INCLUDES="${INCLUDES}extern const unsigned char ${name}_img[];\n"
    OBJS="${OBJS} ${name}.img.o "

    STUB="${STUB} {\"$name\", size_${name}_img, ${name}_img},"
done

STUB="${STUB}};"

echo -e "$INCLUDES" > includes.img.h
echo -e "$STUB" > stub.hpp
echo $OBJS > objnames.img.txt
