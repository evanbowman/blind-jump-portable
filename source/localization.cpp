#include "localization.hpp"
#include "platform/platform.hpp"
#include "script/lisp.hpp"


// Each language should define a texture mapping, which tells the rendering code
// where in the texture data to look for the glyph corresponding to a given utf8
// codepoint. Keep in mind, that on some platforms, like the Gameboy Advance,
// there is very little texture memory available for displaying glyphs. You may
// only be able to display 80 or so unique characters on the screen at a time.
std::optional<Platform::TextureMapping>
standard_texture_map(const utf8::Codepoint& cp)
{
    auto mapping = [&]() -> std::optional<u16> {
        switch (cp) {
        case '0':
            return 1;
        case '1':
            return 2;
        case '2':
            return 3;
        case '3':
            return 4;
        case '4':
            return 5;
        case '5':
            return 6;
        case '6':
            return 7;
        case '7':
            return 8;
        case '8':
            return 9;
        case '9':
            return 10;
        case 'a':
            return 11;
        case 'b':
            return 12;
        case 'c':
            return 13;
        case 'd':
            return 14;
        case 'e':
            return 15;
        case 'f':
            return 16;
        case 'g':
            return 17;
        case 'h':
            return 18;
        case 'i':
            return 19;
        case 'j':
            return 20;
        case 'k':
            return 21;
        case 'l':
            return 22;
        case 'm':
            return 23;
        case 'n':
            return 24;
        case 'o':
            return 25;
        case 'p':
            return 26;
        case 'q':
            return 27;
        case 'r':
            return 28;
        case 's':
            return 29;
        case 't':
            return 30;
        case 'u':
            return 31;
        case 'v':
            return 32;
        case 'w':
            return 33;
        case 'x':
            return 34;
        case 'y':
            return 35;
        case 'z':
            return 36;
        case '.':
            return 37;
        case ',':
            return 38;
        case 'A':
            return 39;
        case 'B':
            return 40;
        case 'C':
            return 41;
        case 'D':
            return 42;
        case 'E':
            return 43;
        case 'F':
            return 44;
        case 'G':
            return 45;
        case 'H':
            return 46;
        case 'I':
            return 47;
        case 'J':
            return 48;
        case 'K':
            return 49;
        case 'L':
            return 50;
        case 'M':
            return 51;
        case 'N':
            return 52;
        case 'O':
            return 53;
        case 'P':
            return 54;
        case 'Q':
            return 55;
        case 'R':
            return 56;
        case 'S':
            return 57;
        case 'T':
            return 58;
        case 'U':
            return 59;
        case 'V':
            return 60;
        case 'W':
            return 61;
        case 'X':
            return 62;
        case 'Y':
            return 63;
        case 'Z':
            return 64;
        case '"':
            return 65;
        case '\'':
            return 66;
        case '[':
            return 67;
        case ']':
            return 68;
        case '(':
            return 69;
        case ')':
            return 70;
        case ':':
            return 71;
        case ' ':
            return 72;
        case '%':
            return 93;
        case '!':
            return 94;
        case '?':
            return 95;
        case '+':
            return 98;
        case '-':
            return 99;
        case '/':
            return 100;
        case '*':
            return 101;
        case '=':
            return 102;
        case '<':
            return 103;
        case '>':
            return 104;
        case '#':
            return 105;
        case '_':
            return 186;
        default:
            if (cp == utf8::getc(u8"©")) {
                return 185;
            }
            // extended spanish and french characters
            else if (cp == utf8::getc(u8"ñ")) {
                return 73;
            } else if (cp == utf8::getc(u8"á")) {
                return 74;
            } else if (cp == utf8::getc(u8"é")) {
                return 75;
            } else if (cp == utf8::getc(u8"í")) {
                return 76;
            } else if (cp == utf8::getc(u8"ó")) {
                return 77;
            } else if (cp == utf8::getc(u8"ú")) {
                return 78;
            } else if (cp == utf8::getc(u8"â")) {
                return 79;
            } else if (cp == utf8::getc(u8"ê")) {
                return 80;
            } else if (cp == utf8::getc(u8"î")) {
                return 81;
            } else if (cp == utf8::getc(u8"ô")) {
                return 82;
            } else if (cp == utf8::getc(u8"û")) {
                return 83;
            } else if (cp == utf8::getc(u8"à")) {
                return 84;
            } else if (cp == utf8::getc(u8"è")) {
                return 85;
            } else if (cp == utf8::getc(u8"ù")) {
                return 86;
            } else if (cp == utf8::getc(u8"ë")) {
                return 87;
            } else if (cp == utf8::getc(u8"ï")) {
                return 88;
            } else if (cp == utf8::getc(u8"ü")) {
                return 89;
            } else if (cp == utf8::getc(u8"ç")) {
                return 90;
            } else if (cp == utf8::getc(u8"Ç")) {
                return 91;
            } else if (cp == utf8::getc(u8"ö")) {
                return 92;
            } else if (cp == utf8::getc(u8"¡")) {
                return 96;
            } else if (cp == utf8::getc(u8"¿")) {
                return 97;
            }
            // katakana
            else if (cp == utf8::getc(u8"ア")) {
                return 106;
            } else if (cp == utf8::getc(u8"イ")) {
                return 107;
            } else if (cp == utf8::getc(u8"ウ")) {
                return 108;
            } else if (cp == utf8::getc(u8"エ")) {
                return 109;
            } else if (cp == utf8::getc(u8"オ")) {
                return 110;
            } else if (cp == utf8::getc(u8"カ")) {
                return 111;
            } else if (cp == utf8::getc(u8"キ")) {
                return 112;
            } else if (cp == utf8::getc(u8"ク")) {
                return 113;
            } else if (cp == utf8::getc(u8"ケ")) {
                return 114;
            } else if (cp == utf8::getc(u8"コ")) {
                return 115;
            } else if (cp == utf8::getc(u8"サ")) {
                return 116;
            } else if (cp == utf8::getc(u8"シ")) {
                return 117;
            } else if (cp == utf8::getc(u8"ス")) {
                return 118;
            } else if (cp == utf8::getc(u8"セ")) {
                return 119;
            } else if (cp == utf8::getc(u8"ソ")) {
                return 120;
            } else if (cp == utf8::getc(u8"タ")) {
                return 121;
            } else if (cp == utf8::getc(u8"チ")) {
                return 122;
            } else if (cp == utf8::getc(u8"ツ")) {
                return 123;
            } else if (cp == utf8::getc(u8"テ")) {
                return 124;
            } else if (cp == utf8::getc(u8"ト")) {
                return 125;
            } else if (cp == utf8::getc(u8"ナ")) {
                return 126;
            } else if (cp == utf8::getc(u8"ニ")) {
                return 127;
            } else if (cp == utf8::getc(u8"ヌ")) {
                return 128;
            } else if (cp == utf8::getc(u8"ネ")) {
                return 129;
            } else if (cp == utf8::getc(u8"ノ")) {
                return 130;
            } else if (cp == utf8::getc(u8"ハ")) {
                return 131;
            } else if (cp == utf8::getc(u8"ヒ")) {
                return 132;
            } else if (cp == utf8::getc(u8"フ")) {
                return 133;
            } else if (cp == utf8::getc(u8"ヘ")) {
                return 134;
            } else if (cp == utf8::getc(u8"ホ")) {
                return 135;
            } else if (cp == utf8::getc(u8"マ")) {
                return 136;
            } else if (cp == utf8::getc(u8"ミ")) {
                return 137;
            } else if (cp == utf8::getc(u8"ム")) {
                return 138;
            } else if (cp == utf8::getc(u8"メ")) {
                return 139;
            } else if (cp == utf8::getc(u8"モ")) {
                return 140;
            } else if (cp == utf8::getc(u8"ヤ")) {
                return 141;
            } else if (cp == utf8::getc(u8"ユ")) {
                return 142;
            } else if (cp == utf8::getc(u8"ヨ")) {
                return 143;
            } else if (cp == utf8::getc(u8"ラ")) {
                return 144;
            } else if (cp == utf8::getc(u8"リ")) {
                return 145;
            } else if (cp == utf8::getc(u8"ル")) {
                return 146;
            } else if (cp == utf8::getc(u8"レ")) {
                return 147;
            } else if (cp == utf8::getc(u8"ロ")) {
                return 148;
            } else if (cp == utf8::getc(u8"ワ")) {
                return 149;
            } else if (cp == utf8::getc(u8"ヲ")) {
                return 150;
            } else if (cp == utf8::getc(u8"ン")) {
                return 151;
            } else if (cp == utf8::getc(u8"ガ")) {
                return 152;
            } else if (cp == utf8::getc(u8"ギ")) {
                return 153;
            } else if (cp == utf8::getc(u8"グ")) {
                return 154;
            } else if (cp == utf8::getc(u8"ゲ")) {
                return 155;
            } else if (cp == utf8::getc(u8"ゴ")) {
                return 156;
            } else if (cp == utf8::getc(u8"ゲ")) {
                return 157;
            } else if (cp == utf8::getc(u8"ジ")) {
                return 158;
            } else if (cp == utf8::getc(u8"ズ")) {
                return 159;
            } else if (cp == utf8::getc(u8"ゼ")) {
                return 160;
            } else if (cp == utf8::getc(u8"ゾ")) {
                return 161;
            } else if (cp == utf8::getc(u8"ダ")) {
                return 162;
            } else if (cp == utf8::getc(u8"ヂ")) {
                return 163;
            } else if (cp == utf8::getc(u8"ヅ")) {
                return 164;
            } else if (cp == utf8::getc(u8"デ")) {
                return 165;
            } else if (cp == utf8::getc(u8"ド")) {
                return 166;
            } else if (cp == utf8::getc(u8"バ")) {
                return 167;
            } else if (cp == utf8::getc(u8"パ")) {
                return 168;
            } else if (cp == utf8::getc(u8"ビ")) {
                return 169;
            } else if (cp == utf8::getc(u8"ピ")) {
                return 170;
            } else if (cp == utf8::getc(u8"ブ")) {
                return 171;
            } else if (cp == utf8::getc(u8"プ")) {
                return 172;
            } else if (cp == utf8::getc(u8"ベ")) {
                return 173;
            } else if (cp == utf8::getc(u8"ペ")) {
                return 174;
            } else if (cp == utf8::getc(u8"ボ")) {
                return 175;
            } else if (cp == utf8::getc(u8"ポ")) {
                return 176;
            } else if (cp == utf8::getc(u8"ー")) {
                return 177;
            } else if (cp == utf8::getc(u8"ヴ")) {
                return 178;
            } else if (cp == utf8::getc(u8"ァ")) {
                return 179;
            } else if (cp == utf8::getc(u8"ィ")) {
                return 180;
            } else if (cp == utf8::getc(u8"ゥ")) {
                return 181;
            } else if (cp == utf8::getc(u8"ェ")) {
                return 182;
            } else if (cp == utf8::getc(u8"ォ")) {
                return 183;
            } else if (cp == utf8::getc(u8"・")) {
                return 184;
            }
            return std::nullopt;
        }
    }();
    if (mapping) {
        return Platform::TextureMapping{"charset", *mapping};
    } else {
        return {};
    }
}


std::optional<Platform::TextureMapping> null_texture_map(const utf8::Codepoint&)
{
    return {};
}


static int language_id = 0;


Platform::TextureCpMapper locale_texture_map()
{
    return standard_texture_map;
}


void locale_set_language(int language_id)
{
    ::language_id = language_id;
}


int locale_get_language()
{
    return ::language_id;
}


StringBuffer<31> locale_language_name(int language)
{
    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, language);

    return lang->expect<lisp::Symbol>().name_;
}


LocalizedText locale_string(Platform& pfrm, LocaleString ls)
{
    auto result = allocate_dynamic<LocalizedStrBuffer>(pfrm);

    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, ::language_id);

    StringBuffer<31> fname = lang->expect<lisp::Symbol>().name_;
    fname += ".txt";

    if (auto data = pfrm.load_file_contents("strings", fname.c_str())) {
        const int target_line = static_cast<int>(ls);

        int index = 0;
        while (index not_eq target_line) {
            while (*data not_eq '\n') {
                if (*data == '\0') {
                    error(pfrm, "blah");
                    while (true)
                        ; // FIXME: raise error...
                }
                ++data;
            }
            ++data;

            ++index;
        }

        while (*data not_eq '\0' and *data not_eq '\n') {
            result->push_back(*data);
            ++data;
        }

        return result;
    } else {
        error(pfrm, "strings file for language does not exist");
        while (true)
            ;
    }
}


void english__to_string(int num, char* buffer, int base)
{
    int i = 0;
    bool is_negative = false;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Based on the behavior of itoa()
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    str_reverse(buffer, i);

    return;
}


void locale_num2str(int num, char* buffer, int base)
{
    // FIXME!!!

    // switch (language) {
    // case LocaleLanguage::spanish:
    // case LocaleLanguage::english:
    english__to_string(num, buffer, base);
    //     break;

    // default:
    // case LocaleLanguage::null:
    // buffer[0] = '\0';
    //     break;
    // }
}
