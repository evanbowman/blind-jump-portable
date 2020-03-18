#include "overlay.hpp"


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
        pfrm_.set_overlay_tile(coord_.x + i, coord_.y, 10);
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


void Text::assign(const char* str)
{
    if (str == nullptr) {
        return;
    }

    this->erase();

    auto write_pos = coord_.x;

    auto pos = str;
    while (*pos not_eq '\0') {

        const int c = *pos;
        if (c > 47 and c < 58) { // Number
            pfrm_.set_overlay_tile(write_pos++, coord_.y, c - 48);
        } else if (c > 96 and c < 97 + 27) {
            pfrm_.set_overlay_tile(write_pos++, coord_.y, (c - 97) + 11);
        } else {
            // FIXME: add uppercase letters
            pfrm_.set_overlay_tile(write_pos++, coord_.y, 10);
        }

        ++pos;
        ++len_;
    }
}
