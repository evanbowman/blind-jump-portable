#include "localization.hpp"
#include "platform/platform.hpp"
#include "script/lisp.hpp"


class str_const {
private:
    const char* const p_;
    const size_t sz_;

public:
    template <size_t N>
    constexpr str_const(const char (&a)[N]) : p_(a), sz_(N - 1)
    {
    }

    constexpr char operator[](std::size_t n)
    {
        return n < sz_ ? p_[n] : '0';
    }
};


// FIXME: This assumes little endian?
// Needs to be a macro because there's no way to pass a str_const as a
// constexpr parameter.
#define UTF8_GETCHR(_STR_)                                                     \
    []() -> utf8::Codepoint {                                                  \
        if constexpr ((str_const(_STR_)[0] & 0x80) == 0) {                     \
            return str_const(_STR_)[0];                                        \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xC0) {           \
            return str_const(_STR_)[0] | ((u32)str_const(_STR_)[1]) << 8;      \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xE0) {           \
            return str_const(_STR_)[0] | ((u32)str_const(_STR_)[1]) << 8 |     \
                   (u32)str_const(_STR_)[2] << 16;                             \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xF0) {           \
            return str_const(_STR_)[0] | (u32)str_const(_STR_)[1] << 8 |       \
                   (u32)str_const(_STR_)[2] << 16 |                            \
                   (u32)str_const(_STR_)[3] << 24;                             \
        } else {                                                               \
            return 0;                                                          \
        }                                                                      \
    }()


std::optional<Platform::TextureMapping>
standard_texture_map(const utf8::Codepoint& cp)
{
    auto mapping = [&]() -> std::optional<u16> {
        switch (cp) {
        // clang-format off
        case UTF8_GETCHR(u8"0"): return 1;
        case UTF8_GETCHR(u8"1"): return 2;
        case UTF8_GETCHR(u8"2"): return 3;
        case UTF8_GETCHR(u8"3"): return 4;
        case UTF8_GETCHR(u8"4"): return 5;
        case UTF8_GETCHR(u8"5"): return 6;
        case UTF8_GETCHR(u8"6"): return 7;
        case UTF8_GETCHR(u8"7"): return 8;
        case UTF8_GETCHR(u8"8"): return 9;
        case UTF8_GETCHR(u8"9"): return 10;
        case UTF8_GETCHR(u8"a"): return 11;
        case UTF8_GETCHR(u8"b"): return 12;
        case UTF8_GETCHR(u8"c"): return 13;
        case UTF8_GETCHR(u8"d"): return 14;
        case UTF8_GETCHR(u8"e"): return 15;
        case UTF8_GETCHR(u8"f"): return 16;
        case UTF8_GETCHR(u8"g"): return 17;
        case UTF8_GETCHR(u8"h"): return 18;
        case UTF8_GETCHR(u8"i"): return 19;
        case UTF8_GETCHR(u8"j"): return 20;
        case UTF8_GETCHR(u8"k"): return 21;
        case UTF8_GETCHR(u8"l"): return 22;
        case UTF8_GETCHR(u8"m"): return 23;
        case UTF8_GETCHR(u8"n"): return 24;
        case UTF8_GETCHR(u8"o"): return 25;
        case UTF8_GETCHR(u8"p"): return 26;
        case UTF8_GETCHR(u8"q"): return 27;
        case UTF8_GETCHR(u8"r"): return 28;
        case UTF8_GETCHR(u8"s"): return 29;
        case UTF8_GETCHR(u8"t"): return 30;
        case UTF8_GETCHR(u8"u"): return 31;
        case UTF8_GETCHR(u8"v"): return 32;
        case UTF8_GETCHR(u8"w"): return 33;
        case UTF8_GETCHR(u8"x"): return 34;
        case UTF8_GETCHR(u8"y"): return 35;
        case UTF8_GETCHR(u8"z"): return 36;
        case UTF8_GETCHR(u8"."): return 37;
        case UTF8_GETCHR(u8","): return 38;
        case UTF8_GETCHR(u8"A"): return 39;
        case UTF8_GETCHR(u8"B"): return 40;
        case UTF8_GETCHR(u8"C"): return 41;
        case UTF8_GETCHR(u8"D"): return 42;
        case UTF8_GETCHR(u8"E"): return 43;
        case UTF8_GETCHR(u8"F"): return 44;
        case UTF8_GETCHR(u8"G"): return 45;
        case UTF8_GETCHR(u8"H"): return 46;
        case UTF8_GETCHR(u8"I"): return 47;
        case UTF8_GETCHR(u8"J"): return 48;
        case UTF8_GETCHR(u8"K"): return 49;
        case UTF8_GETCHR(u8"L"): return 50;
        case UTF8_GETCHR(u8"M"): return 51;
        case UTF8_GETCHR(u8"N"): return 52;
        case UTF8_GETCHR(u8"O"): return 53;
        case UTF8_GETCHR(u8"P"): return 54;
        case UTF8_GETCHR(u8"Q"): return 55;
        case UTF8_GETCHR(u8"R"): return 56;
        case UTF8_GETCHR(u8"S"): return 57;
        case UTF8_GETCHR(u8"T"): return 58;
        case UTF8_GETCHR(u8"U"): return 59;
        case UTF8_GETCHR(u8"V"): return 60;
        case UTF8_GETCHR(u8"W"): return 61;
        case UTF8_GETCHR(u8"X"): return 62;
        case UTF8_GETCHR(u8"Y"): return 63;
        case UTF8_GETCHR(u8"Z"): return 64;
        case UTF8_GETCHR(u8"\""): return 65;
        case UTF8_GETCHR(u8"'"): return 66;
        case UTF8_GETCHR(u8"["): return 67;
        case UTF8_GETCHR(u8"]"): return 68;
        case UTF8_GETCHR(u8"("): return 69;
        case UTF8_GETCHR(u8")"): return 70;
        case UTF8_GETCHR(u8":"): return 71;
        case UTF8_GETCHR(u8" "): return 72;
        case UTF8_GETCHR(u8"%"): return 93;
        case UTF8_GETCHR(u8"!"): return 94;
        case UTF8_GETCHR(u8"?"): return 95;
        case UTF8_GETCHR(u8"+"): return 98;
        case UTF8_GETCHR(u8"-"): return 99;
        case UTF8_GETCHR(u8"/"): return 100;
        case UTF8_GETCHR(u8"*"): return 101;
        case UTF8_GETCHR(u8"="): return 102;
        case UTF8_GETCHR(u8"<"): return 103;
        case UTF8_GETCHR(u8">"): return 104;
        case UTF8_GETCHR(u8"#"): return 105;
        case UTF8_GETCHR(u8"_"): return 186;

        // Chinese Glyphs:
        case UTF8_GETCHR(u8"星"): return 187;
        case UTF8_GETCHR(u8"际"): return 191;
        case UTF8_GETCHR(u8"跃"): return 195;
        case UTF8_GETCHR(u8"动"): return 199;
        case UTF8_GETCHR(u8"新"): return 203;
        case UTF8_GETCHR(u8"游"): return 207;
        case UTF8_GETCHR(u8"戏"): return 211;
        case UTF8_GETCHR(u8"继"): return 215;
        case UTF8_GETCHR(u8"续"): return 219;
        case UTF8_GETCHR(u8"制"): return 223;
        case UTF8_GETCHR(u8"作"): return 227;
        case UTF8_GETCHR(u8"你"): return 231;
        case UTF8_GETCHR(u8"敌"): return 235;
        case UTF8_GETCHR(u8"人"): return 239;
        case UTF8_GETCHR(u8"传"): return 243;
        case UTF8_GETCHR(u8"送"): return 247;
        case UTF8_GETCHR(u8"点"): return 251;
        case UTF8_GETCHR(u8"物"): return 255;
        case UTF8_GETCHR(u8"品"): return 259;
        case UTF8_GETCHR(u8"的"): return 263;
        case UTF8_GETCHR(u8"开"): return 267;
        case UTF8_GETCHR(u8"始"): return 271;
        case UTF8_GETCHR(u8"商"): return 275;
        case UTF8_GETCHR(u8"店"): return 279;
        case UTF8_GETCHR(u8"章"): return 283;
        case UTF8_GETCHR(u8"节"): return 287;
        case UTF8_GETCHR(u8"到"): return 291;
        case UTF8_GETCHR(u8"达"): return 295;
        case UTF8_GETCHR(u8"者"): return 299;
        case UTF8_GETCHR(u8"一"): return 303;
        case UTF8_GETCHR(u8"二"): return 307;
        case UTF8_GETCHR(u8"三"): return 311;
        case UTF8_GETCHR(u8"四"): return 315;
        case UTF8_GETCHR(u8"勇"): return 319;
        case UTF8_GETCHR(u8"往"): return 323;
        case UTF8_GETCHR(u8"直"): return 327;
        case UTF8_GETCHR(u8"下"): return 331;
        case UTF8_GETCHR(u8"肮"): return 335;
        case UTF8_GETCHR(u8"脏"): return 339;
        case UTF8_GETCHR(u8"之"): return 343;
        case UTF8_GETCHR(u8"处"): return 347;
        case UTF8_GETCHR(u8"月"): return 351;
        case UTF8_GETCHR(u8"光"): return 355;
        case UTF8_GETCHR(u8"空"): return 359;
        case UTF8_GETCHR(u8"白"): return 363;
        case UTF8_GETCHR(u8"曾"): return 367;
        case UTF8_GETCHR(u8"经"): return 371;
        case UTF8_GETCHR(u8"海"): return 375;
        case UTF8_GETCHR(u8"报"): return 379;
        case UTF8_GETCHR(u8"工"): return 383;
        case UTF8_GETCHR(u8"笔"): return 387;
        case UTF8_GETCHR(u8"记"): return 391;
        case UTF8_GETCHR(u8"程"): return 395;
        case UTF8_GETCHR(u8"师"): return 399;
        case UTF8_GETCHR(u8"爆"): return 403;
        case UTF8_GETCHR(u8"破"): return 407;
        case UTF8_GETCHR(u8"器"): return 411;
        case UTF8_GETCHR(u8"加"): return 415;
        case UTF8_GETCHR(u8"速"): return 419;
        case UTF8_GETCHR(u8"放"): return 423;
        case UTF8_GETCHR(u8"慢"): return 427;
        case UTF8_GETCHR(u8"系"): return 431;
        case UTF8_GETCHR(u8"统"): return 435;
        case UTF8_GETCHR(u8"地"): return 439;
        case UTF8_GETCHR(u8"图"): return 443;
        case UTF8_GETCHR(u8"炸"): return 447;
        case UTF8_GETCHR(u8"弹"): return 451;
        case UTF8_GETCHR(u8"电"): return 455;
        case UTF8_GETCHR(u8"磁"): return 459;
        case UTF8_GETCHR(u8"干"): return 463;
        case UTF8_GETCHR(u8"扰"): return 467;
        case UTF8_GETCHR(u8"种"): return 471;
        case UTF8_GETCHR(u8"子"): return 475;
        case UTF8_GETCHR(u8"袋"): return 479;
        case UTF8_GETCHR(u8"导"): return 483;
        case UTF8_GETCHR(u8"航"): return 487;
        case UTF8_GETCHR(u8"册"): return 491;
        case UTF8_GETCHR(u8"桔"): return 495;
        case UTF8_GETCHR(u8"籽"): return 499;
        // NOTE: All glyphs for the first 35 lines completed.

        case UTF8_GETCHR(u8"仅"): return 503;
        case UTF8_GETCHR(u8"能"): return 507;
        case UTF8_GETCHR(u8"使"): return 511;
        case UTF8_GETCHR(u8"用"): return 515;
        case UTF8_GETCHR(u8"次"): return 519;
        // Intentional gap.
        case UTF8_GETCHR(u8"得"): return 527;
        case UTF8_GETCHR(u8"了"): return 531;
        case UTF8_GETCHR(u8"离"): return 535;
        case UTF8_GETCHR(u8"们"): return 539;


            // clang-format on

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


// I had to add this code during chinese translation, for places where I needed
// to use traditional chinese numbers rather than arabic numerals.
const char* locale_repr_smallnum(u8 num, std::array<char, 40>& buffer)
{
    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, ::language_id);

    const char* lang_name =
        lang->expect<lisp::Cons>().car()->expect<lisp::Symbol>().name_;

    if (str_cmp(lang_name, "chinese")) {
        switch (num) {
        default:
        case 1: return "一";
        case 2: return "二";
        case 3: return "三";
        case 4: return "四";
        }
    } else {
        locale_num2str(num, buffer.data(), 10);
        return buffer.data();
    }
}


int locale_get_language()
{
    return ::language_id;
}


StringBuffer<31> locale_language_name(int language)
{
    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, language);

    return lang->expect<lisp::Cons>().car()->expect<lisp::Symbol>().name_;
}


bool locale_requires_doublesize_font()
{
    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, ::language_id);

    return lang->expect<lisp::Cons>()
               .cdr()
               ->expect<lisp::Cons>()
               .car()
               ->expect<lisp::Integer>()
               .value_ == 2;
}


LocalizedText locale_string(Platform& pfrm, LocaleString ls)
{
    auto result = allocate_dynamic<LocalizedStrBuffer>(pfrm);

    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, ::language_id);

    StringBuffer<31> fname =
        lang->expect<lisp::Cons>().car()->expect<lisp::Symbol>().name_;
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
