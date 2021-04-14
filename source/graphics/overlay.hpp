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


struct FontConfiguration {

    // Double-sized text configuration added during chinese language
    // localization, when we determined that chinese glyphs would not be
    // legible at 8x8 sizes. When set to true, the text engine will use four
    // tiles per glyph, i.e. each glyph will be 16x16.
    bool double_size_ = false;
};


class Text {
public:
    Text(Platform& pfrm,
         const char* str,
         const OverlayCoord& coord,
         const FontConfiguration& config = {});

    Text(Platform& pfrm,
         const OverlayCoord& coord,
         const FontConfiguration& config = {});

    Text(const Text&) = delete;
    Text(Text&&);

    ~Text();

    using OptColors = std::optional<FontColors>;

    void assign(const char* str, const OptColors& colors = {});
    void assign(int num, const OptColors& colors = {});

    void append(const char* str, const OptColors& colors = {});
    void append(int num, const OptColors& colors = {});

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

    const FontConfiguration& config() const
    {
        return config_;
    }

private:
    void resize(u32 len);

    Platform& pfrm_;
    const OverlayCoord coord_;
    Length len_;
    const FontConfiguration config_;
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
           int tile_offset = 0,
           TileDesc default_tile = 0);
    Border(const Border&) = delete;
    ~Border();

private:
    Platform& pfrm_;
    OverlayCoord size_;
    OverlayCoord position_;
    bool filled_;
    TileDesc default_tile_;
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


class LoadingBar {
public:
    LoadingBar(Platform& pfrm, u8 width, const OverlayCoord& position);
    LoadingBar(const LoadingBar&) = delete;
    ~LoadingBar();

    void set_progress(Float percentage);

private:
    Platform& pfrm_;
    OverlayCoord position_;
    u8 width_;
};


// Swoops in/out from the right side of the screen, based on display percentage.
class Sidebar {
public:
    Sidebar(Platform& pfrm, u8 width, u8 height, const OverlayCoord& pos);
    Sidebar(const Sidebar&) = delete;
    ~Sidebar();

    void set_display_percentage(Float percentage);

private:
    Platform& pfrm_;
    const u8 width_;
    const u8 height_;
    OverlayCoord pos_;
};


class LeftSidebar {
public:
    LeftSidebar(Platform& pfrm, u8 width, u8 height, const OverlayCoord& pos);
    LeftSidebar(const Sidebar&) = delete;
    ~LeftSidebar();

    void set_display_percentage(Float percentage);

private:
    Platform& pfrm_;
    const u8 width_;
    const u8 height_;
    OverlayCoord pos_;
};


class HorizontalFlashAnimation {
public:
    HorizontalFlashAnimation(Platform& pfrm, const OverlayCoord& position)
        : pfrm_(pfrm), position_(position), width_(0), timer_(0), index_(0)
    {
    }

    HorizontalFlashAnimation(const HorizontalFlashAnimation&) = delete;

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

    void set_position(const OverlayCoord& position)
    {
        position_ = position;
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


enum class Resolution {
    r16_9,
    r3_2,
    unknown,
    count,
};


inline Resolution resolution(Platform::Screen& screen)
{
    if (screen.size().x == 240 and screen.size().y == 160) {
        return Resolution::r3_2;
    } else if (screen.size().x == 240 and screen.size().y == 136) {
        return Resolution::r16_9;
    }
    return Resolution::unknown;
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
    enum class Align { left, right };


    inline UIMetric(Platform& pfrm,
                    const OverlayCoord& pos,
                    int icon_tile,
                    int value,
                    Align align)
        : icon_tile_(icon_tile), value_(value), anim_(pfrm, pos), align_(align),
          pos_(pos)
    {
        display(pfrm);
    }

    UIMetric(const UIMetric&) = delete;

    virtual ~UIMetric()
    {
    }

    inline void set_value(int value)
    {
        const auto value_len = integer_text_length(value);

        anim_.init(value_len);
        value_ = value;


        switch (align_) {
        case Align::left:
            anim_.set_position({u8(pos_.x + 1), pos_.y});
            break;

        case Align::right: {
            anim_.set_position({u8(pos_.x - value_len), pos_.y});
            break;
        }
        }
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

    virtual void on_display(Text&, int)
    {
    }

    const OverlayCoord& position() const
    {
        return pos_;
    }

private:
    inline void display(Platform& pfrm)
    {
        switch (align_) {
        case Align::left: {
            icon_.emplace(pfrm, icon_tile_, pos_);
            text_.emplace(pfrm, OverlayCoord{u8(pos_.x + 1), pos_.y});
            break;
        }

        case Align::right: {
            const auto len = integer_text_length(value_);
            icon_.emplace(pfrm, icon_tile_, pos_);
            text_.emplace(pfrm, OverlayCoord{u8(pos_.x - len), pos_.y});
            break;
        }
        }

        text_->assign(value_);

        on_display(*text_, value_);
    }

    const int icon_tile_;
    std::optional<SmallIcon> icon_;
    std::optional<Text> text_;
    int value_;
    HorizontalFlashAnimation anim_;
    const Align align_;
    OverlayCoord pos_;
};


template <typename F> void instrument(Platform& pfrm, F&& callback)
{
    static int index;
    constexpr int sample_count = 32;
    static int buffer[32];
    static std::optional<Text> text;

    const auto before = pfrm.delta_clock().sample();

    callback();

    const auto after = pfrm.delta_clock().sample();

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
        text->assign(Platform::DeltaClock::duration(before, after));
        text->append(" micros");
    }
}
