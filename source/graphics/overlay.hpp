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
    Text(Platform& pfrm, const char* str, const OverlayCoord& coord);
    Text(Platform& pfrm, const OverlayCoord& coord);
    Text(const Text&) = delete;

    ~Text();

    void assign(const char* str);
    void assign(int num);

    void append(const char* str);
    void append(int num);

    void erase();

    using Length = u16;

    Length len()
    {
        return len_;
    }

private:
    Platform& pfrm_;
    const OverlayCoord coord_;
    Length len_;
};


// 8x8 pixels
class SmallIcon {
public:
    SmallIcon(Platform& pfrm, int tile, const OverlayCoord& coord);
    SmallIcon(const SmallIcon&) = delete;
    ~SmallIcon();

private:
    Platform& pfrm_;
    const OverlayCoord coord_;
};


// 16x16 pixels, comprised of four 8x8 tiles
class MediumIcon {
public:
    MediumIcon(Platform& pfrm, int tile, const OverlayCoord& coord);
    MediumIcon(const MediumIcon&) = delete;
    ~MediumIcon();

private:
    Platform& pfrm_;
    const OverlayCoord coord_;
};


// Unlike Text, TextView understands words, reflows words onto new lines, and is
// capable of scrolling vertically through a block of text.
class TextView {
public:
    TextView(Platform& pfrm);
    TextView(const TextView&) = delete;
    ~TextView();

    // Use the skiplines parameter to scroll the textview vertically.
    void assign(const char* str,
                const OverlayCoord& coord,
                const OverlayCoord& size,
                int skiplines = 0);

    inline const OverlayCoord& size()
    {
        return size_;
    }

    // Returns the amount of the input string was processed by the textview
    // rendering code. Useful for determining whether adding extra skiplines to
    // the assign() function would scroll the text further.
    size_t parsed() const
    {
        return parsed_;
    }

private:
    Platform& pfrm_;
    OverlayCoord size_;
    OverlayCoord position_;
    size_t parsed_;
};


class Border {
public:
    Border(Platform& pfrm,
           const OverlayCoord& size,
           const OverlayCoord& position,
           bool fill = false,
           int tile_offset = 0);
    Border(const Border&) = delete;
    ~Border();

private:
    Platform& pfrm_;
    OverlayCoord size_;
    OverlayCoord position_;
    bool filled_;
};


class DottedHorizontalLine {
public:
    DottedHorizontalLine(Platform& pfrm, u8 y);
    DottedHorizontalLine(const DottedHorizontalLine& other) = delete;
    ~DottedHorizontalLine();

private:
    Platform& pfrm_;
    u8 y_;
};


inline OverlayCoord calc_screen_tiles(Platform& pfrm)
{
    constexpr u32 overlay_tile_size = 8;
    return (pfrm.screen().size() / overlay_tile_size).cast<u8>();
}


using Margin = u16;


inline Margin centered_text_margins(Platform& pfrm, u16 text_length)
{
    const auto width = calc_screen_tiles(pfrm).x;

    return (width - text_length) / 2;
}


inline void left_text_margin(Text& text, Margin margin)
{
    for (int i = 0; i < margin; ++i) {
        text.append(" ");
    }
}


inline void right_text_margin(Text& text, Margin margin)
{
    for (int i = 0; i < margin + 1 /* due to rounding in margin calc */; ++i) {
        text.append(" ");
    }
}


u32 integer_text_length(int n);
