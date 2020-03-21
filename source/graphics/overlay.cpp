#include "overlay.hpp"


size_t strlen(const char* str)
{
    const char* s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}


static void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        std::swap(*(str + start), *(str + end));
        start++;
        end--;
    }
}


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
    reverse(str, i);

    return str;
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


void Text::assign(int val, Style style)
{
    std::array<char, 40> buffer = {0};

    this->assign(myitoa(val, buffer.data(), 10), style);
}


static void
print_char(Platform& pfrm, char c, const OverlayCoord& coord, Text::Style style)
{
    const auto offset = [&] {
        switch (style) {
        case Text::Style::old_paper:
            return 39;
        default:
            return 0;
        };
    }();

    if (c > 47 and c < 58) { // Number
        pfrm.set_overlay_tile(coord.x, coord.y, offset + (c - 48) + 1);
    } else if (c > 96 and c < 97 + 27) {
        pfrm.set_overlay_tile(coord.x, coord.y, offset + (c - 97) + 11);
    } else if (c == ',') {
        pfrm.set_overlay_tile(coord.x, coord.y, offset + 38);
    } else if (c == '.') {
        pfrm.set_overlay_tile(coord.x, coord.y, offset + 37);
    } else {
        // FIXME: add uppercase letters
        pfrm.set_overlay_tile(coord.x, coord.y, offset + 0);
    }
}


void Text::assign(const char* str, Style style)
{
    if (str == nullptr) {
        return;
    }

    this->erase();

    auto write_pos = coord_.x;

    auto pos = str;
    while (*pos not_eq '\0') {

        print_char(pfrm_, *pos, {write_pos, coord_.y}, style);

        ++write_pos;
        ++pos;
        ++len_;
    }
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

    const auto len = strlen(str);

    auto cursor = coord;

    auto newline = [&] {
        while (cursor.x < size.x) {
            print_char(pfrm_, ' ', cursor, Text::Style::old_paper);
            ++cursor.x;
        }
        cursor.x = coord.x;
        ++cursor.y;
    };

    for (size_t i = 0; i < len; ++i) {

        if (cursor.x == size.x) {
            if (str[i] not_eq ' ') {
                // If the next character is not a space, then the current word
                // does not fit on the current line, and needs to be written
                // onto the next line instead.
                while (str[i] not_eq ' ') {
                    --i;
                    --cursor.x;
                }
                newline();
                if (cursor.y == size.y - 1) {
                    break;
                }
                newline();
            } else {
                cursor.y += 1;
                cursor.x = coord.x;
                if (cursor.y == size.y - 1) {
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
                print_char(pfrm_, str[i], cursor, Text::Style::old_paper);
            }

            ++cursor.x;
        }
    }

    while (cursor.y < size.y) {
        newline();
    }
}
