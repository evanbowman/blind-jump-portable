#pragma once


#include "platform/platform.hpp"


// A collection of display routines for printing various things to the game's
// overlay drawing layer. The game draws the overlay with the highest level of
// priority, so everything else, both entities and the game map, will be drawn
// beneath. Unlike entities or the overworld map, the overlay is not
// positionable, but instead, purely tile-based. The overlay uses absolute
// coordinates, so when the screen's view scrolls to different areas of the
// overworld, the overlay stays put.
//
// The overlay uses 8x8 tiles; smaller than any other graphics element. But a UI
// sometimes demands more flexibility and precision than sprites and
// backgrounds, e.g.: displaying text.


using OverlayCoord = Vec2<u8>;


class Text {
public:
    // Setting a custom font color is sort of platform specific, and resource
    // intensive on some platforms. So we provide a few presents.
    enum class Style { default_red, old_paper };

    Text(Platform& pfrm, const char* str, const OverlayCoord& coord);
    Text(Platform& pfrm, const OverlayCoord& coord);
    Text(const Text&) = delete;

    ~Text();

    void assign(const char* str, Style = Style::default_red);
    void assign(int num, Style = Style::default_red);

    void erase();

private:
    Platform& pfrm_;
    const OverlayCoord coord_;
    u16 len_;
};


// Unlike Text, TextView understands words, reflows words onto new lines, and is
// capable of scrolling vertically through a block of text.
class TextView {
public:
    TextView(Platform& pfrm);

    // Use the skiplines parameter to scroll the textview vertically.
    void assign(const char* str,
                const OverlayCoord& coord,
                const OverlayCoord& size,
                int skiplines = 0);

private:
    Platform& pfrm_;
};
