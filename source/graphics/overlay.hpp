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
    Text(Text&&);

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

    const OverlayCoord& coord() const
    {
        return coord_;
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

    inline const auto& coord() const
    {
        return coord_;
    }

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


class BossHealthBar {
public:
    BossHealthBar(Platform& pfrm, u8 height, const OverlayCoord& position);
    BossHealthBar(const BossHealthBar&) = delete;
    ~BossHealthBar();

    void set_health(Float percentage);

private:
    Platform& pfrm_;
    OverlayCoord position_;
    u8 height_;
};


class HorizontalFlashAnimation {
public:
    HorizontalFlashAnimation(Platform& pfrm, const OverlayCoord& position)
        : pfrm_(pfrm), position_(position), width_(0), timer_(0), index_(0)
    {
    }

    HorizontalFlashAnimation(HorizontalFlashAnimation&) = delete;

    ~HorizontalFlashAnimation()
    {
        fill(0);
    }

    void init(int width)
    {
        width_ = width;
        timer_ = 0;
        index_ = 0;
    }

    enum { anim_len = 3 };

    void update(Microseconds dt)
    {
        timer_ += dt;
        if (timer_ > milliseconds(34)) {
            timer_ = 0;
            if (done()) {
                return;
            }
            if (index_ < anim_len) {
                fill(108 + index_);
            }
            ++index_;
        }
    }

    bool done() const
    {
        return index_ == anim_len + 1;
    }

    const auto& position() const
    {
        return position_;
    }

private:

    void fill(int tile)
    {
        for (int i = position_.x; i < position_.x + width_; ++i) {
            pfrm_.set_tile(Layer::overlay, i, position_.y, tile);
        }
    }

    Platform& pfrm_;
    OverlayCoord position_;
    int width_;
    int timer_;
    int index_;
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


class UIMetric {
public:
    inline UIMetric(Platform& pfrm,
             const OverlayCoord& pos,
             int icon_tile,
             int value) :
        icon_tile_(icon_tile),
        value_(value),
        anim_(pfrm, pos)
    {
        display(pfrm);
    }

    UIMetric(const UIMetric&) = delete;

    virtual ~UIMetric() {}

    inline void set_value(int value)
    {
        value_ = value;
        anim_.init(integer_text_length(value) + 1);
    }

    inline void update(Platform& pfrm, Microseconds dt)
    {
        if (not anim_.done()) {
            anim_.update(dt);
            if (anim_.done()) {
                display(pfrm);
            }
        }
    }

    virtual void on_display(Text& text, int value) {}

private:

    inline void display(Platform& pfrm)
    {
        const auto pos = anim_.position();
        icon_.emplace(pfrm, icon_tile_, pos);
        text_.emplace(pfrm, OverlayCoord{u8(pos.x + 1), pos.y});
        text_->assign(value_);
        on_display(*text_, value_);
    }

    const int icon_tile_;
    std::optional<SmallIcon> icon_;
    std::optional<Text> text_;
    int value_;
    HorizontalFlashAnimation anim_;
};


template <typename F> void instrument(Platform& pfrm, F&& callback)
{
    static int index;
    constexpr int sample_count = 32;
    static int buffer[32];
    static std::optional<Text> text;

    const auto before = DeltaClock::instance().sample();

    callback();

    const auto after = DeltaClock::instance().sample();

    if (index < sample_count) {
        buffer[index++] = after - before;

    } else {
        index = 0;

        int accum = 0;
        for (int i = 0; i < sample_count; ++i) {
            accum += buffer[i];
        }

        accum /= 32;

        text.emplace(pfrm, OverlayCoord{1, 1});
        text->assign(DeltaClock::duration(before, after));
        text->append(" micros");
    }
}
