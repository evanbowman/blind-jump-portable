#include "overlay.hpp"
#include "localization.hpp"
#include "string.hpp"


u32 integer_text_length(int n)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(n, buffer.data(), 10);

    return str_len(buffer.data());
}


Text::Text(Platform& pfrm, const char* str, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord), len_(0)
{
    this->assign(str);
}


Text::Text(Platform& pfrm, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord), len_(0)
{
}


void Text::erase()
{
    for (int i = 0; i < len_; ++i) {
        pfrm_.set_tile(Layer::overlay, coord_.x + i, coord_.y, 0);
    }

    len_ = 0;
}


Text::Text(Text&& from)
    : pfrm_(from.pfrm_), coord_(from.coord_), len_(from.len_)
{
    from.len_ = 0;
}


Text::~Text()
{
    this->erase();
}


void Text::assign(int val)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(val, buffer.data(), 10);
    this->assign(buffer.data());
}


Platform::TextureCpMapper locale_texture_map();


static void
print_char(Platform& pfrm, utf8::Codepoint c, const OverlayCoord& coord)
{
    if (c not_eq 0) {
        const auto t = pfrm.map_glyph(c, locale_texture_map());

        pfrm.set_tile(Layer::overlay, coord.x, coord.y, t);
    } else {
        pfrm.set_tile(Layer::overlay, coord.x, coord.y, 0);
    }
}


void Text::assign(const char* str)
{
    this->erase();

    this->append(str);
}


void Text::append(const char* str)
{
    if (str == nullptr or not validate_str(str)) {
        return;
    }

    auto write_pos = static_cast<u8>(coord_.x + len_);

    utf8::scan(
        [&](const utf8::Codepoint& cp, const char* raw) {
            print_char(pfrm_, cp, {write_pos, coord_.y});
            ++write_pos;
            ++len_;
        },
        str,
        str_len(str));
}


void Text::append(int num)
{
    std::array<char, 40> buffer = {0};

    locale_num2str(num, buffer.data(), 10);
    this->append(buffer.data());
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


TextView::TextView(Platform& pfrm) : pfrm_(pfrm)
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


class BufferedUnicodeStr {
private:
    const char* const str_;

    static constexpr const int index_count_ = 300;
    utf8::Codepoint data_[index_count_];
    int index_start_;
    int str_len_;

    void load_chunk(int index)
    {
        int i = 0;
        utf8::scan(
            [this, &i, &index](const utf8::Codepoint& cp, const char*) {
                if (i >= index and i < index + index_count_) {
                    data_[i - index] = cp;
                }
                i++;
            },
            str_,
            str_len_);

        index_start_ = index;
    }

public:
    BufferedUnicodeStr(const char* str, int len)
        : str_(str), index_start_(0), str_len_(len)
    {
        load_chunk(0);
    }

    utf8::Codepoint get(int index)
    {
        if (not(index >= index_start_ and
                index < index_start_ + index_count_)) {
            load_chunk(index);
        }

        return data_[index - index_start_];
    }
};


void TextView::assign(const char* str,
                      const OverlayCoord& coord,
                      const OverlayCoord& size,
                      int skiplines)
{
    position_ = coord;
    size_ = size;

    const auto len = str_len(str);
    BufferedUnicodeStr ustr(str, len);

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
    for (i = 0; i < len; ++i) {

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
               int tile_offset)
    : pfrm_(pfrm), size_(size), position_(position), filled_(fill)
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
                pfrm_.set_tile(Layer::overlay, x, y, 0);

            } else if (x == position_.x) {
                pfrm_.set_tile(Layer::overlay, x, y, 0);

            } else if (y == position_.y) {
                pfrm_.set_tile(Layer::overlay, x, y, 0);

            } else if (x == stopx - 1) {
                pfrm_.set_tile(Layer::overlay, x, y, 0);

            } else if (y == stopy - 1) {
                pfrm_.set_tile(Layer::overlay, x, y, 0);

            } else if (filled_) {
                pfrm_.set_tile(Layer::overlay, x, y, 0);
            }
        }
    }
}


DottedHorizontalLine::DottedHorizontalLine(Platform& pfrm, u8 y)
    : pfrm_(pfrm), y_(y)
{
    for (int i = 0; i < 30; ++i) {
        pfrm.set_tile(Layer::overlay, i, y, 109);
    }
}


DottedHorizontalLine::~DottedHorizontalLine()
{
    for (int i = 0; i < 30; ++i) {
        pfrm_.set_tile(Layer::overlay, i, y_, 0);
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
