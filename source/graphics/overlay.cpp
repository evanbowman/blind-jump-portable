#include "overlay.hpp"
#include "localization.hpp"
#include "string.hpp"


u32 integer_text_length(int n)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(n, buffer.data(), 10);

    return str_len(buffer.data());
}


Text::Text(Platform& pfrm,
           const char* str,
           const OverlayCoord& coord,
           const FontConfiguration& config)
    : pfrm_(pfrm), coord_(coord), len_(0), config_(config)
{
    this->assign(str);
}


Text::Text(Platform& pfrm,
           const OverlayCoord& coord,
           const FontConfiguration& config)
    : pfrm_(pfrm), coord_(coord), len_(0), config_(config)
{
}


void Text::erase()
{
    if (not config_.double_size_) {
        for (int i = 0; i < len_; ++i) {
            pfrm_.set_tile(Layer::overlay, coord_.x + i, coord_.y, 0);
        }
    } else {
        for (int i = 0; i < len_ * 2; ++i) {
            pfrm_.set_tile(Layer::overlay, coord_.x + i, coord_.y, 0);
        }
        for (int i = 0; i < len_ * 2; ++i) {
            pfrm_.set_tile(Layer::overlay, coord_.x + i, coord_.y + 1, 0);
        }
    }

    len_ = 0;
}


Text::Text(Text&& from)
    : pfrm_(from.pfrm_), coord_(from.coord_), len_(from.len_),
      config_(from.config_)
{
    from.len_ = 0;
}


Text::~Text()
{
    this->erase();
}


void Text::assign(int val, const OptColors& colors)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(val, buffer.data(), 10);
    this->assign(buffer.data(), colors);
}


Platform::TextureCpMapper locale_texture_map();
Platform::TextureCpMapper locale_doublesize_texture_map();


void print_double_char(Platform& pfrm,
                       utf8::Codepoint c,
                       const OverlayCoord& coord,
                       const std::optional<FontColors>& colors = {})
{
    if (c not_eq 0) {
        const auto mapping_info = locale_doublesize_texture_map()(c);

        u16 t0 = 495;
        u16 t1 = 495;
        u16 t2 = 495;
        u16 t3 = 495;

        if (mapping_info) {
            auto info = *mapping_info;

            // FIXME: these special cases should be handled in the texture map
            // lookup.
            if (info.offset_ == 72) {
                t0 = pfrm.map_glyph(c, info);
                // Special case for space character
                t1 = t0;
                t2 = t0;
                t3 = t0;
            } else if (info.offset_ == 65) {
                // Special case for enlarged quote character
                info.offset_ = 523;
                t0 = pfrm.map_glyph(c, info);
                info.offset_ = 524;
                t1 = pfrm.map_glyph(c, info);
                info.offset_ = 72; // space
                t2 = pfrm.map_glyph(c, info);
                t3 = pfrm.map_glyph(c, info);
            } else if (info.offset_ == 38 or info.offset_ == 37) {
                // Special case for period, comma
                t2 = pfrm.map_glyph(c, info);
                info.offset_ = 72; // space
                t0 = pfrm.map_glyph(c, info);
                t1 = pfrm.map_glyph(c, info);
                t3 = pfrm.map_glyph(c, info);
            } else {
                t0 = pfrm.map_glyph(c, info);
                info.offset_++;
                t1 = pfrm.map_glyph(c, info);
                info.offset_++;
                t2 = pfrm.map_glyph(c, info);
                info.offset_++;
                t3 = pfrm.map_glyph(c, info);
            }
        }

        if (not colors) {
            pfrm.set_tile(Layer::overlay, coord.x, coord.y, t0);
            pfrm.set_tile(Layer::overlay, coord.x + 1, coord.y, t1);
            pfrm.set_tile(Layer::overlay, coord.x, coord.y + 1, t2);
            pfrm.set_tile(Layer::overlay, coord.x + 1, coord.y + 1, t3);
        } else {
            pfrm.set_tile(coord.x, coord.y, t0, *colors);
            pfrm.set_tile(coord.x + 1, coord.y, t1, *colors);
            pfrm.set_tile(coord.x, coord.y + 1, t2, *colors);
            pfrm.set_tile(coord.x + 1, coord.y + 1, t3, *colors);
        }
    } else {
        pfrm.set_tile(Layer::overlay, coord.x, coord.y, 0);
        pfrm.set_tile(Layer::overlay, coord.x + 1, coord.y, 0);
        pfrm.set_tile(Layer::overlay, coord.x, coord.y + 1, 0);
        pfrm.set_tile(Layer::overlay, coord.x + 1, coord.y + 1, 0);
    }
}


static void print_char(Platform& pfrm,
                       utf8::Codepoint c,
                       const OverlayCoord& coord,
                       const std::optional<FontColors>& colors = {})
{
    if (c not_eq 0) {
        auto mapping_info = locale_texture_map()(c);

        u16 t = 495;

        if (mapping_info) {

            switch (mapping_info->offset_) {
            }


            t = pfrm.map_glyph(c, *mapping_info);
        }

        if (not colors) {
            pfrm.set_tile(Layer::overlay, coord.x, coord.y, t);
        } else {
            pfrm.set_tile(coord.x, coord.y, t, *colors);
        }
    } else {
        pfrm.set_tile(Layer::overlay, coord.x, coord.y, 0);
    }
}


void Text::resize(u32 len)
{
    for (int i = len - 1; i < this->len(); ++i) {
        pfrm_.set_tile(Layer::overlay, coord_.x + i, coord_.y, 0);
    }
    len_ = 0;
}


void Text::assign(const char* str, const OptColors& colors)
{
    // this->resize(utf8::len(str));
    this->erase();

    this->append(str, colors);
}


void Text::append(const char* str, const OptColors& colors)
{
    if (str == nullptr or not validate_str(str)) {
        return;
    }

    if (config_.double_size_) {
        auto write_pos = static_cast<u8>(coord_.x + len_ * 2);

        utf8::scan(
            [&](const utf8::Codepoint& cp, const char* raw, int) {
                print_double_char(pfrm_, cp, {write_pos, coord_.y}, colors);
                write_pos += 2;
                ++len_;
            },
            str,
            str_len(str));

    } else {
        auto write_pos = static_cast<u8>(coord_.x + len_);

        utf8::scan(
            [&](const utf8::Codepoint& cp, const char* raw, int) {
                print_char(pfrm_, cp, {write_pos, coord_.y}, colors);
                ++write_pos;
                ++len_;
            },
            str,
            str_len(str));
    }
}


void Text::append(int num, const OptColors& colors)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(num, buffer.data(), 10);
    this->append(buffer.data(), colors);
}


SmallIcon::SmallIcon(Platform& pfrm, int tile, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord)
{
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y, tile);
}


SmallIcon::~SmallIcon()
{
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y, 0);
}


MediumIcon::MediumIcon(Platform& pfrm, int tile, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord)
{
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y, tile);
    pfrm_.set_tile(Layer::overlay, coord_.x + 1, coord_.y, tile + 1);
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y + 1, tile + 2);
    pfrm_.set_tile(Layer::overlay, coord_.x + 1, coord_.y + 1, tile + 3);
}


MediumIcon::~MediumIcon()
{
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y, 0);
    pfrm_.set_tile(Layer::overlay, coord_.x + 1, coord_.y, 0);
    pfrm_.set_tile(Layer::overlay, coord_.x, coord_.y + 1, 0);
    pfrm_.set_tile(Layer::overlay, coord_.x + 1, coord_.y + 1, 0);
}


TextView::TextView(Platform& pfrm) : pfrm_(pfrm), parsed_(0)
{
}

TextView::~TextView()
{
    for (int i = position_.x; i < position_.x + size_.x; ++i) {
        for (int j = position_.y; j < position_.y + size_.y; ++j) {
            pfrm_.set_tile(Layer::overlay, i, j, 0);
        }
    }
}


void TextView::assign(const char* str,
                      const OverlayCoord& coord,
                      const OverlayCoord& size,
                      int skiplines)
{
    position_ = coord;
    size_ = size;

    const auto len = str_len(str);
    const auto ulen = utf8::len(str);
    utf8::BufferedStr ustr(str, len);

    auto cursor = coord;

    auto newline = [&] {
        while (cursor.x < coord.x + size.x) {
            print_char(pfrm_, ' ', cursor);
            ++cursor.x;
        }
        cursor.x = coord.x;
        ++cursor.y;
    };

    size_t i;
    for (i = 0; i < ulen; ++i) {

        if (cursor.x == coord.x + size.x) {
            if (ustr.get(i) not_eq ' ') {
                // If the next character is not a space, then the current word
                // does not fit on the current line, and needs to be written
                // onto the next line instead.
                while (ustr.get(i) not_eq ' ') {
                    --i;
                    --cursor.x;
                }
                newline();
                if (cursor.y == (coord.y + size.y) - 1) {
                    break;
                }
                newline();
            } else {
                cursor.y += 1;
                cursor.x = coord.x;
                if (cursor.y == (coord.y + size.y) - 1) {
                    break;
                }
                newline();
            }
            if (skiplines) {
                --skiplines;
                cursor.y -= 2;
            }
        }

        if (cursor.x == coord.x and ustr.get(i) == ' ') {
            // ...
        } else {
            if (not skiplines) {
                print_char(pfrm_, ustr.get(i), cursor);
            }

            ++cursor.x;
        }
    }

    parsed_ = i;

    while (cursor.y < (coord.y + size.y)) {
        newline();
    }
}


Border::Border(Platform& pfrm,
               const OverlayCoord& size,
               const OverlayCoord& position,
               bool fill,
               int tile_offset,
               TileDesc default_tile)
    : pfrm_(pfrm), size_(size), position_(position), filled_(fill),
      default_tile_(default_tile)
{
    const auto stopx = position_.x + size_.x;
    const auto stopy = position_.y + size_.y;

    for (int x = position_.x; x < stopx; ++x) {
        for (int y = position_.y; y < stopy; ++y) {

            if (x == position_.x and y == position_.y) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 86 + tile_offset);

            } else if (x == position_.x and y == stopy - 1) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 88 + tile_offset);

            } else if (x == stopx - 1 and y == position_.y) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 85 + tile_offset);

            } else if (x == stopx - 1 and y == stopy - 1) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 87 + tile_offset);

            } else if (x == position_.x) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 84 + tile_offset);

            } else if (y == position_.y) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 81 + tile_offset);

            } else if (x == stopx - 1) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 82 + tile_offset);

            } else if (y == stopy - 1) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 83 + tile_offset);

            } else if (fill) {
                pfrm.set_tile(Layer::overlay, x, y, 67 + 80);
            }
        }
    }
}


Border::~Border()
{
    const auto stopx = position_.x + size_.x;
    const auto stopy = position_.y + size_.y;

    for (int x = position_.x; x < stopx; ++x) {
        for (int y = position_.y; y < stopy; ++y) {

            // TODO: simplify this if/else
            if ((x == position_.x and y == position_.y) or
                (x == position_.x and y == stopy - 1) or
                (x == stopx - 1 and y == position_.y) or
                (x == stopx - 1 and y == stopy - 1)) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);

            } else if (x == position_.x) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);

            } else if (y == position_.y) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);

            } else if (x == stopx - 1) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);

            } else if (y == stopy - 1) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);

            } else if (filled_) {
                pfrm_.set_tile(Layer::overlay, x, y, default_tile_);
            }
        }
    }
}


BossHealthBar::BossHealthBar(Platform& pfrm,
                             u8 height,
                             const OverlayCoord& position)
    : pfrm_(pfrm), position_(position), height_(height)
{
    pfrm_.set_tile(Layer::overlay, position_.x, position_.y, 82);
    pfrm_.set_tile(Layer::overlay, position_.x, position_.y + height + 1, 83);
    set_health(0.f);
}


void BossHealthBar::set_health(Float percentage)
{
    constexpr int pixels_per_tile = 8;
    const auto total_pixels = height_ * pixels_per_tile;

    int fractional_pixels = percentage * total_pixels;
    int current_tile = 0;

    while (fractional_pixels >= 8) {
        pfrm_.set_tile(
            Layer::overlay, position_.x, position_.y + 1 + current_tile, 91);
        fractional_pixels -= 8;
        ++current_tile;
    }

    if (current_tile < height_ and fractional_pixels % 8 not_eq 0) {
        pfrm_.set_tile(Layer::overlay,
                       position_.x,
                       position_.y + 1 + current_tile,
                       83 + fractional_pixels % 8);
        ++current_tile;
    }

    while (current_tile < height_) {
        pfrm_.set_tile(
            Layer::overlay, position_.x, position_.y + 1 + current_tile, 92);
        ++current_tile;
    }
}


BossHealthBar::~BossHealthBar()
{
    for (int y = 0; y < height_ + 2 /* +2 due to the header and footer */;
         ++y) {
        pfrm_.set_tile(Layer::overlay, position_.x, position_.y + y, 0);
    }
}


LoadingBar::LoadingBar(Platform& pfrm, u8 width, const OverlayCoord& position)
    : pfrm_(pfrm), position_(position), width_(width)
{
    pfrm_.set_tile(Layer::overlay, position_.x, position_.y, 401);
    pfrm_.set_tile(Layer::overlay, position_.x + width + 1, position_.y, 411);

    set_progress(0.f);
}


LoadingBar::~LoadingBar()
{
    for (int x = 0; x < width_ + 5; ++x) {
        pfrm_.set_tile(Layer::overlay, position_.x + x, position_.y, 0);
    }
}


void LoadingBar::set_progress(Float percentage)
{
    constexpr int pixels_per_tile = 8;
    const auto total_pixels = width_ * pixels_per_tile;

    int fractional_pixels = percentage * total_pixels;
    int current_tile = 0;

    while (fractional_pixels >= 8) {
        pfrm_.set_tile(
            Layer::overlay, position_.x + 1 + current_tile, position_.y, 410);
        fractional_pixels -= 8;
        ++current_tile;
    }

    if (current_tile < width_ and fractional_pixels % 8 not_eq 0) {
        pfrm_.set_tile(Layer::overlay,
                       position_.x + 1 + current_tile,
                       position_.y,
                       402 + fractional_pixels % 8);
        ++current_tile;
    }

    while (current_tile < width_) {
        pfrm_.set_tile(
            Layer::overlay, position_.x + 1 + current_tile, position_.y, 402);
        ++current_tile;
    }
}


Sidebar::Sidebar(Platform& pfrm, u8 width, u8 height, const OverlayCoord& pos)
    : pfrm_(pfrm), width_(width), height_(height), pos_(pos)
{
}


void Sidebar::set_display_percentage(Float percentage)
{
    constexpr int pixels_per_tile = 8;
    const auto total_pixels = width_ * pixels_per_tile;

    const int fractional_pixels = percentage * total_pixels;

    // const auto screen_tiles = calc_screen_tiles(pfrm_);

    for (int y = pos_.y; y < pos_.y + height_; ++y) {
        int pixels = fractional_pixels;

        int current_tile = 0;

        while (pixels >= 8) {
            pfrm_.set_tile(Layer::overlay, pos_.x - (1 + current_tile), y, 121);
            pixels -= 8;
            ++current_tile;
        }

        if (current_tile < width_ and pixels % 8 not_eq 0) {
            pfrm_.set_tile(Layer::overlay,
                           pos_.x - (1 + current_tile),
                           y,
                           128 - pixels % 8);
            ++current_tile;
        }

        while (current_tile < width_) {
            pfrm_.set_tile(Layer::overlay, pos_.x - (1 + current_tile), y, 0);
            ++current_tile;
        }
    }
}


Sidebar::~Sidebar()
{
    set_display_percentage(0.f);
}


LeftSidebar::LeftSidebar(Platform& pfrm,
                         u8 width,
                         u8 height,
                         const OverlayCoord& pos)
    : pfrm_(pfrm), width_(width), height_(height), pos_(pos)
{
}


void LeftSidebar::set_display_percentage(Float percentage)
{
    constexpr int pixels_per_tile = 8;
    const auto total_pixels = width_ * pixels_per_tile;

    const int fractional_pixels = percentage * total_pixels;

    // const auto screen_tiles = calc_screen_tiles(pfrm_);

    for (int y = pos_.y; y < pos_.y + height_; ++y) {
        int pixels = fractional_pixels;

        int current_tile = 0;

        while (pixels >= 8) {
            pfrm_.set_tile(Layer::overlay, current_tile + pos_.x, y, 121);
            pixels -= 8;
            ++current_tile;
        }

        if (current_tile < width_ and pixels % 8 not_eq 0) {
            pfrm_.set_tile(
                Layer::overlay, current_tile + pos_.x, y, 433 - pixels % 8);
            ++current_tile;
        }

        while (current_tile < width_) {
            pfrm_.set_tile(Layer::overlay, current_tile + pos_.x, y, 0);
            ++current_tile;
        }
    }
}


LeftSidebar::~LeftSidebar()
{
    set_display_percentage(0.f);
}
