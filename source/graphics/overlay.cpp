#include "overlay.hpp"
#include "string.hpp"


// Implementation of itoa()
static char* myitoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    str_reverse(str, i);

    return str;
}


u32 integer_text_length(int n)
{
    std::array<char, 40> buffer = {0};

    myitoa(n, buffer.data(), 10);

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
        pfrm_.set_overlay_tile(coord_.x + i, coord_.y, 0);
    }

    len_ = 0;
}


Text::~Text()
{
    this->erase();
}


void Text::assign(int val)
{
    std::array<char, 40> buffer = {0};

    this->assign(myitoa(val, buffer.data(), 10));
}


static void print_char(Platform& pfrm, char c, const OverlayCoord& coord)
{
    if (c > 47 and c < 58) { // Number
        pfrm.set_overlay_tile(coord.x, coord.y, (c - 48) + 1);
    } else if (c > 96 and c < 97 + 27) {
        pfrm.set_overlay_tile(coord.x, coord.y, (c - 97) + 11);
    } else if (c == ',') {
        pfrm.set_overlay_tile(coord.x, coord.y, 38);
    } else if (c == '.') {
        pfrm.set_overlay_tile(coord.x, coord.y, 37);
    } else if (c == '\'') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 1);
    } else if (c == '[') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 2);
    } else if (c == ']') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 3);
    } else if (c == '(') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 4);
    } else if (c == ')') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 5);
    } else if (c == '"') {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26);
    } else if (c > 64 and c < 65 + 27) {
        // FIXME: add uppercase letters
        pfrm.set_overlay_tile(coord.x, coord.y, (c - 65) + 39);
    } else {
        pfrm.set_overlay_tile(coord.x, coord.y, 39 + 26 + 6);
    }
}


void Text::assign(const char* str)
{
    this->erase();

    this->append(str);
}


void Text::append(const char* str)
{
    if (str == nullptr) {
        return;
    }

    auto write_pos = static_cast<u8>(coord_.x + len_);

    auto pos = str;
    while (*pos not_eq '\0') {

        print_char(pfrm_, *pos, {write_pos, coord_.y});

        ++write_pos;
        ++pos;
        ++len_;
    }
}


void Text::append(int num)
{
    std::array<char, 40> buffer = {0};

    this->append(myitoa(num, buffer.data(), 10));
}


SmallIcon::SmallIcon(Platform& pfrm, int tile, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord)
{
    pfrm_.set_overlay_tile(coord_.x, coord_.y, tile);
}


SmallIcon::~SmallIcon()
{
    pfrm_.set_overlay_tile(coord_.x, coord_.y, 0);
}


MediumIcon::MediumIcon(Platform& pfrm, int tile, const OverlayCoord& coord)
    : pfrm_(pfrm), coord_(coord)
{
    pfrm_.set_overlay_tile(coord_.x, coord_.y, tile);
    pfrm_.set_overlay_tile(coord_.x + 1, coord_.y, tile + 1);
    pfrm_.set_overlay_tile(coord_.x, coord_.y + 1, tile + 2);
    pfrm_.set_overlay_tile(coord_.x + 1, coord_.y + 1, tile + 3);
}


MediumIcon::~MediumIcon()
{
    pfrm_.set_overlay_tile(coord_.x, coord_.y, 0);
    pfrm_.set_overlay_tile(coord_.x + 1, coord_.y, 0);
    pfrm_.set_overlay_tile(coord_.x, coord_.y + 1, 0);
    pfrm_.set_overlay_tile(coord_.x + 1, coord_.y + 1, 0);
}


TextView::TextView(Platform& pfrm) : pfrm_(pfrm)
{
}

TextView::~TextView()
{
    for (int i = position_.x; i < position_.x + size_.x; ++i) {
        for (int j = position_.y; j < position_.y + size_.y; ++j) {
            pfrm_.set_overlay_tile(i, j, 0);
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
            if (str[i] not_eq ' ') {
                // If the next character is not a space, then the current word
                // does not fit on the current line, and needs to be written
                // onto the next line instead.
                while (str[i] not_eq ' ') {
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

        if (cursor.x == coord.x and str[i] == ' ') {
            // ...
        } else {
            if (not skiplines) {
                print_char(pfrm_, str[i], cursor);
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
                pfrm.set_overlay_tile(x, y, 67 + 86 + tile_offset);

            } else if (x == position_.x and y == stopy - 1) {
                pfrm.set_overlay_tile(x, y, 67 + 88 + tile_offset);

            } else if (x == stopx - 1 and y == position_.y) {
                pfrm.set_overlay_tile(x, y, 67 + 85 + tile_offset);

            } else if (x == stopx - 1 and y == stopy - 1) {
                pfrm.set_overlay_tile(x, y, 67 + 87 + tile_offset);

            } else if (x == position_.x) {
                pfrm.set_overlay_tile(x, y, 67 + 84 + tile_offset);

            } else if (y == position_.y) {
                pfrm.set_overlay_tile(x, y, 67 + 81 + tile_offset);

            } else if (x == stopx - 1) {
                pfrm.set_overlay_tile(x, y, 67 + 82 + tile_offset);

            } else if (y == stopy - 1) {
                pfrm.set_overlay_tile(x, y, 67 + 83 + tile_offset);

            } else if (fill) {
                pfrm.set_overlay_tile(x, y, 67 + 80);
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
                pfrm_.set_overlay_tile(x, y, 0);

            } else if (x == position_.x) {
                pfrm_.set_overlay_tile(x, y, 0);

            } else if (y == position_.y) {
                pfrm_.set_overlay_tile(x, y, 0);

            } else if (x == stopx - 1) {
                pfrm_.set_overlay_tile(x, y, 0);

            } else if (y == stopy - 1) {
                pfrm_.set_overlay_tile(x, y, 0);

            } else if (filled_) {
                pfrm_.set_overlay_tile(x, y, 0);
            }
        }
    }
}


DottedHorizontalLine::DottedHorizontalLine(Platform& pfrm, u8 y)
    : pfrm_(pfrm), y_(y)
{
    for (int i = 0; i < 30; ++i) {
        pfrm.set_overlay_tile(i, y, 109);
    }
}


DottedHorizontalLine::~DottedHorizontalLine()
{
    for (int i = 0; i < 30; ++i) {
        pfrm_.set_overlay_tile(i, y_, 0);
    }
}


BossHealthBar::BossHealthBar(Platform& pfrm,
                             u8 height,
                             const OverlayCoord& position)
    : pfrm_(pfrm), position_(position), height_(height)
{
    pfrm_.set_overlay_tile(position_.x, position_.y, 81);
    pfrm_.set_overlay_tile(position_.x, position_.y + height, 82);
    set_health(0.f);
}


void BossHealthBar::set_health(Float percentage)
{
    constexpr int pixels_per_tile = 8;
    const auto total_pixels = height_ * pixels_per_tile;

    int fractional_pixels = percentage * total_pixels;
    int current_tile = 0;

    while (fractional_pixels >= 8) {
        pfrm_.set_overlay_tile(position_.x, position_.y + 1 + current_tile, 90);
        fractional_pixels -= 8;
        ++current_tile;
    }

    if (current_tile < height_ + 1 and fractional_pixels % 8 not_eq 0) {
        pfrm_.set_overlay_tile(position_.x,
                               position_.y + 1 + current_tile,
                               82 + fractional_pixels % 8);
        ++current_tile;
    }

    while (current_tile < height_ + 1) {
        pfrm_.set_overlay_tile(position_.x, position_.y + 1 + current_tile, 91);
        ++current_tile;
    }
}


BossHealthBar::~BossHealthBar()
{
    for (int y = 0; y < height_ + 2 /* +2 due to the header and footer */;
         ++y) {
        pfrm_.set_overlay_tile(position_.x, position_.y + y, 0);
    }
}
