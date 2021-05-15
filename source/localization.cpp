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


#ifndef __BYTE_ORDER__
#error "byte order must be defined"
#endif


// FIXME: assumes little endian? Does it matter though, which way we order
// stuff, as long as it's consistent? Actually it does matter, considering
// that we're byte-swapping stuff in unicode.hpp
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error "TODO: fix the utf-8 decoding (below) for big endian"
#endif


// Needs to be a macro because there's no way to pass a str_const as a
// constexpr parameter. Converts the first utf-8 codepoint in a string to a
// 32-bit integer, for use in a giant switch statement (below).
#define UTF8_GETCHR(_STR_)                                                     \
    []() -> utf8::Codepoint {                                                  \
        if constexpr ((str_const(_STR_)[0] & 0x80) == 0) {                     \
            return str_const(_STR_)[0];                                        \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xC0 ||           \
                             (str_const(_STR_)[0] & 0xf0) == 0xD0) {           \
            return (u32)(u8)str_const(_STR_)[0] |                              \
                   (((u32)(u8)str_const(_STR_)[1]) << 8);                      \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xE0) {           \
            return (u32)(u8)str_const(_STR_)[0] |                              \
                   (((u32)(u8)str_const(_STR_)[1]) << 8) |                     \
                   ((u32)(u8)str_const(_STR_)[2] << 16);                       \
        } else if constexpr ((str_const(_STR_)[0] & 0xf0) == 0xF0) {           \
            return (u32)(u8)str_const(_STR_)[0] |                              \
                   ((u32)(u8)str_const(_STR_)[1] << 8) |                       \
                   ((u32)(u8)str_const(_STR_)[2] << 16) |                      \
                   ((u32)(u8)str_const(_STR_)[3] << 24);                       \
        } else {                                                               \
            return 0;                                                          \
        }                                                                      \
    }()


template <u32 B, bool C> constexpr void my_assert()
{
    static_assert(C, "oh no");
}

#define UTF8_TESTCHR(_STR_)                                                    \
    []() -> utf8::Codepoint {                                                  \
        my_assert<(u32)(u8)str_const(_STR_)[0], false>();                      \
        return 0;                                                              \
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

        // Cyrillic Characters
        case UTF8_GETCHR(u8"А"): return 2085;
        case UTF8_GETCHR(u8"Б"): return 2086;
        case UTF8_GETCHR(u8"В"): return 2087;
        case UTF8_GETCHR(u8"Г"): return 2088;
        case UTF8_GETCHR(u8"Д"): return 2089;
        case UTF8_GETCHR(u8"Е"): return 2090;
        case UTF8_GETCHR(u8"Ж"): return 2091;
        case UTF8_GETCHR(u8"З"): return 2092;
        case UTF8_GETCHR(u8"И"): return 2093;
        case UTF8_GETCHR(u8"Й"): return 2094;
        case UTF8_GETCHR(u8"К"): return 2095;
        case UTF8_GETCHR(u8"Л"): return 2096;
        case UTF8_GETCHR(u8"М"): return 2097;
        case UTF8_GETCHR(u8"Н"): return 2098;
        case UTF8_GETCHR(u8"О"): return 2099;
        case UTF8_GETCHR(u8"П"): return 2100;
        case UTF8_GETCHR(u8"Р"): return 2101;
        case UTF8_GETCHR(u8"С"): return 2102;
        case UTF8_GETCHR(u8"Т"): return 2103;
        case UTF8_GETCHR(u8"У"): return 2104;
        case UTF8_GETCHR(u8"Ф"): return 2105;
        case UTF8_GETCHR(u8"Х"): return 2106;
        case UTF8_GETCHR(u8"Ц"): return 2107;
        case UTF8_GETCHR(u8"Ч"): return 2108;
        case UTF8_GETCHR(u8"Ш"): return 2109;
        case UTF8_GETCHR(u8"Щ"): return 2110;
        case UTF8_GETCHR(u8"Ъ"): return 2111;
        case UTF8_GETCHR(u8"Ы"): return 2112;
        case UTF8_GETCHR(u8"Ь"): return 2113;
        case UTF8_GETCHR(u8"Э"): return 2114;
        case UTF8_GETCHR(u8"Ю"): return 2115;
        case UTF8_GETCHR(u8"Я"): return 2116;
        case UTF8_GETCHR(u8"а"): return 2117;
        case UTF8_GETCHR(u8"б"): return 2118;
        case UTF8_GETCHR(u8"в"): return 2119;
        case UTF8_GETCHR(u8"г"): return 2120;
        case UTF8_GETCHR(u8"д"): return 2121;
        case UTF8_GETCHR(u8"е"): return 2122;
        case UTF8_GETCHR(u8"ж"): return 2123;
        case UTF8_GETCHR(u8"з"): return 2124;
        case UTF8_GETCHR(u8"и"): return 2125;
        case UTF8_GETCHR(u8"й"): return 2126;
        case UTF8_GETCHR(u8"к"): return 2127;
        case UTF8_GETCHR(u8"л"): return 2128;
        case UTF8_GETCHR(u8"м"): return 2129;
        case UTF8_GETCHR(u8"н"): return 2130;
        case UTF8_GETCHR(u8"о"): return 2131;
        case UTF8_GETCHR(u8"п"): return 2132;
        case UTF8_GETCHR(u8"р"): return 2133;
        case UTF8_GETCHR(u8"с"): return 2134;
        case UTF8_GETCHR(u8"т"): return 2135;
        case UTF8_GETCHR(u8"у"): return 2136;
        case UTF8_GETCHR(u8"ф"): return 2137;
        case UTF8_GETCHR(u8"х"): return 2138;
        case UTF8_GETCHR(u8"ц"): return 2139;
        case UTF8_GETCHR(u8"ч"): return 2140;
        case UTF8_GETCHR(u8"ш"): return 2141;
        case UTF8_GETCHR(u8"щ"): return 2142;
        case UTF8_GETCHR(u8"ъ"): return 2143;
        case UTF8_GETCHR(u8"ы"): return 2144;
        case UTF8_GETCHR(u8"ь"): return 2145;
        case UTF8_GETCHR(u8"э"): return 2146;
        case UTF8_GETCHR(u8"ю"): return 2147;
        case UTF8_GETCHR(u8"я"): return 2148;
        case UTF8_GETCHR(u8"ё"): return 87;


        // A small number of tiny Chinese glyphs. We don't use too many, because
        // they're difficult to read at this size.
        case UTF8_GETCHR(u8"分"): return 1807;
        case UTF8_GETCHR(u8"数"): return 1808;
        case UTF8_GETCHR(u8"层"): return 1809;
        case UTF8_GETCHR(u8"物"): return 1810;
        case UTF8_GETCHR(u8"品"): return 1811;
        case UTF8_GETCHR(u8"收"): return 1812;
        case UTF8_GETCHR(u8"集"): return 1813;
        case UTF8_GETCHR(u8"程"): return 1814;
        case UTF8_GETCHR(u8"度"): return 1815;
        case UTF8_GETCHR(u8"章"): return 1816;
        case UTF8_GETCHR(u8"节"): return 1817;
        case UTF8_GETCHR(u8"时"): return 1818;
        case UTF8_GETCHR(u8"间"): return 1819;
        case UTF8_GETCHR(u8"一"): return 1828;
        case UTF8_GETCHR(u8"二"): return 1829;
        case UTF8_GETCHR(u8"三"): return 1830;
        case UTF8_GETCHR(u8"四"): return 1831;
        case UTF8_GETCHR(u8"英"): return 1836;
        case UTF8_GETCHR(u8"尺"): return 1837;
        case UTF8_GETCHR(u8"米"): return 2070;

            // clang-format on

        default:
            return std::nullopt;
        }
    }();
    if (mapping) {
        return Platform::TextureMapping{"charset", *mapping};
    } else {
        return {};
    }
}


std::optional<Platform::TextureMapping>
doublesize_texture_map(const utf8::Codepoint& cp)
{
    auto mapping = [&]() -> std::optional<u16> {
        switch (cp) {
            // clang-format off
        case UTF8_GETCHR(u8"0"): return 1763;
        case UTF8_GETCHR(u8"1"): return 1767;
        case UTF8_GETCHR(u8"2"): return 1771;
        case UTF8_GETCHR(u8"3"): return 1775;
        case UTF8_GETCHR(u8"4"): return 1779;
        case UTF8_GETCHR(u8"5"): return 1783;
        case UTF8_GETCHR(u8"6"): return 1787;
        case UTF8_GETCHR(u8"7"): return 1791;
        case UTF8_GETCHR(u8"8"): return 1795;
        case UTF8_GETCHR(u8"9"): return 1799;

        // FIXME: This block includes regular-sized glyphs.
        case UTF8_GETCHR(u8"."): return 37;
        case UTF8_GETCHR(u8","): return 38;
        case UTF8_GETCHR(u8"\""): return 65;
        case UTF8_GETCHR(u8"'"): return 66;
        case UTF8_GETCHR(u8"["): return 1755;
        case UTF8_GETCHR(u8"]"): return 1759;
        case UTF8_GETCHR(u8"("): return 69;
        case UTF8_GETCHR(u8")"): return 70;
        case UTF8_GETCHR(u8":"): return 743;
        case UTF8_GETCHR(u8" "): return 72;
        case UTF8_GETCHR(u8"!"): return 835;
        case UTF8_GETCHR(u8"?"): return 1259;
        case UTF8_GETCHR(u8"+"): return 1902;
        case UTF8_GETCHR(u8"-"): return 1906;
        case UTF8_GETCHR(u8"/"): return 100;
        case UTF8_GETCHR(u8"*"): return 101;
        case UTF8_GETCHR(u8"="): return 102;
        case UTF8_GETCHR(u8"<"): return 103;
        case UTF8_GETCHR(u8">"): return 104;
        case UTF8_GETCHR(u8"#"): return 105;
        case UTF8_GETCHR(u8"_"): return 186;


        case UTF8_GETCHR(u8"A"): return 1910;
        case UTF8_GETCHR(u8"B"): return 1914;


        case UTF8_GETCHR(u8"%"): return 1832;

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
        case UTF8_GETCHR(u8"什"): return 543;
        case UTF8_GETCHR(u8"么"): return 547;
        case UTF8_GETCHR(u8"都"): return 551;
        case UTF8_GETCHR(u8"没"): return 555;
        case UTF8_GETCHR(u8"有"): return 559;
        case UTF8_GETCHR(u8"库"): return 563;
        case UTF8_GETCHR(u8"存"): return 567;
        case UTF8_GETCHR(u8"已"): return 571;
        case UTF8_GETCHR(u8"满"): return 575;
        case UTF8_GETCHR(u8"休"): return 579;
        case UTF8_GETCHR(u8"息"): return 583;
        case UTF8_GETCHR(u8"分"): return 587;
        case UTF8_GETCHR(u8"数"): return 591;
        case UTF8_GETCHR(u8"高"): return 595;
        case UTF8_GETCHR(u8"最"): return 599;
        case UTF8_GETCHR(u8"时"): return 603;
        case UTF8_GETCHR(u8"间"): return 607;
        case UTF8_GETCHR(u8"返"): return 611;
        case UTF8_GETCHR(u8"回"): return 615;
        case UTF8_GETCHR(u8"联"): return 619;
        case UTF8_GETCHR(u8"机"): return 623;
        case UTF8_GETCHR(u8"设"): return 627;
        case UTF8_GETCHR(u8"置"): return 631;
        case UTF8_GETCHR(u8"控"): return 635;
        case UTF8_GETCHR(u8"台"): return 639;
        case UTF8_GETCHR(u8"退"): return 643;
        case UTF8_GETCHR(u8"出"): return 647;
        case UTF8_GETCHR(u8"并"): return 651;
        case UTF8_GETCHR(u8"保"): return 655;
        case UTF8_GETCHR(u8"咱"): return 659;
        case UTF8_GETCHR(u8"见"): return 663;
        case UTF8_GETCHR(u8"过"): return 667;
        case UTF8_GETCHR(u8"会"): return 671;
        case UTF8_GETCHR(u8"抵"): return 675;
        case UTF8_GETCHR(u8"深"): return 679;
        case UTF8_GETCHR(u8"荒"): return 683;
        case UTF8_GETCHR(u8"废"): return 687;
        case UTF8_GETCHR(u8"上"): return 691;
        case UTF8_GETCHR(u8"锁"): return 695;
        case UTF8_GETCHR(u8"跳"): return 699;
        // Some misc number glyphs...
        case UTF8_GETCHR(u8"五"): return 703;
        case UTF8_GETCHR(u8"六"): return 707;
        case UTF8_GETCHR(u8"七"): return 711;
        case UTF8_GETCHR(u8"八"): return 715;
        case UTF8_GETCHR(u8"九"): return 719;
        case UTF8_GETCHR(u8"十"): return 723;
        // For settings text:
        case UTF8_GETCHR(u8"按"): return 727;
        case UTF8_GETCHR(u8"钮"): return 731;
        case UTF8_GETCHR(u8"反"): return 735;
        case UTF8_GETCHR(u8"转"): return 739;
        // Intentional break
        case UTF8_GETCHR(u8"语"): return 747;
        case UTF8_GETCHR(u8"言"): return 751;
        case UTF8_GETCHR(u8"确"): return 755;
        case UTF8_GETCHR(u8"定"): return 759;
        case UTF8_GETCHR(u8"不"): return 763;
        case UTF8_GETCHR(u8"中"): return 767;
        case UTF8_GETCHR(u8"文"): return 771;
        // Boss Death text:
        case UTF8_GETCHR(u8"流"): return 775;
        case UTF8_GETCHR(u8"浪"): return 779;
        case UTF8_GETCHR(u8"被"): return 783;
        case UTF8_GETCHR(u8"打"): return 787;
        case UTF8_GETCHR(u8"败"): return 791;
        case UTF8_GETCHR(u8"守"): return 795;
        case UTF8_GETCHR(u8"门"): return 799;
        case UTF8_GETCHR(u8"摧"): return 803;
        case UTF8_GETCHR(u8"毁"): return 807;
        // At line 128
        case UTF8_GETCHR(u8"检"): return 811;
        case UTF8_GETCHR(u8"测"): return 815;
        case UTF8_GETCHR(u8"波"): return 819;
        // ... 128 ...
        // At line 138
        case UTF8_GETCHR(u8"大"): return 823;
        case UTF8_GETCHR(u8"逼"): return 827;
        case UTF8_GETCHR(u8"近"): return 831;
        // ... 138 ...
        // Intentional break
        case UTF8_GETCHR(u8"选"): return 839;
        case UTF8_GETCHR(u8"择"): return 843;
        case UTF8_GETCHR(u8"目"): return 847;
        case UTF8_GETCHR(u8"标"): return 851;
        case UTF8_GETCHR(u8"菜"): return 855;
        case UTF8_GETCHR(u8"单"): return 859;
        case UTF8_GETCHR(u8"禁"): return 863;
        case UTF8_GETCHR(u8"队"): return 867;
        case UTF8_GETCHR(u8"友"): return 871;
        case UTF8_GETCHR(u8"正"): return 875;
        case UTF8_GETCHR(u8"在"): return 879;
        case UTF8_GETCHR(u8"连"): return 883;
        case UTF8_GETCHR(u8"接"): return 887;
        case UTF8_GETCHR(u8"失"): return 891;
        case UTF8_GETCHR(u8"死"): return 895;
        case UTF8_GETCHR(u8"同"): return 899;
        case UTF8_GETCHR(u8"伴"): return 903;
        case UTF8_GETCHR(u8"生"): return 907;
        case UTF8_GETCHR(u8"命"): return 911;
        case UTF8_GETCHR(u8"更"): return 915;
        case UTF8_GETCHR(u8"改"): return 919;
        case UTF8_GETCHR(u8"为"): return 923;
        case UTF8_GETCHR(u8"等"): return 927;
        case UTF8_GETCHR(u8"待"): return 931;
        case UTF8_GETCHR(u8"步"): return 935;
        case UTF8_GETCHR(u8"需"): return 939;
        case UTF8_GETCHR(u8"要"): return 943;
        case UTF8_GETCHR(u8"双"): return 947;
        case UTF8_GETCHR(u8"兄"): return 951;
        case UTF8_GETCHR(u8"弟"): return 955;
        case UTF8_GETCHR(u8"受"): return 959;
        case UTF8_GETCHR(u8"污"): return 963;
        case UTF8_GETCHR(u8"染"): return 967;
        case UTF8_GETCHR(u8"管"): return 971;
        case UTF8_GETCHR(u8"道"): return 975;
        case UTF8_GETCHR(u8"清"): return 979;
        case UTF8_GETCHR(u8"洗"): return 983;
        case UTF8_GETCHR(u8"购"): return 987;
        case UTF8_GETCHR(u8"买"): return 991;
        case UTF8_GETCHR(u8"售"): return 995;
        case UTF8_GETCHR(u8"价"): return 999;
        case UTF8_GETCHR(u8"格"): return 1003;
        case UTF8_GETCHR(u8"信"): return 1007;
        case UTF8_GETCHR(u8"卖"): return 1011;
        case UTF8_GETCHR(u8"总"): return 1015;
        case UTF8_GETCHR(u8"体"): return 1019;
        case UTF8_GETCHR(u8"成"): return 1023;
        case UTF8_GETCHR(u8"绩"): return 1027;
        case UTF8_GETCHR(u8"聊"): return 1031;
        case UTF8_GETCHR(u8"天"): return 1035;
        case UTF8_GETCHR(u8"志"): return 1039;
        case UTF8_GETCHR(u8"好"): return 1043;
        case UTF8_GETCHR(u8"吗"): return 1047;
        case UTF8_GETCHR(u8"我"): return 1051;
        case UTF8_GETCHR(u8"发"): return 1055;
        case UTF8_GETCHR(u8"这"): return 1059;
        case UTF8_GETCHR(u8"里"): return 1063;
        case UTF8_GETCHR(u8"可"): return 1067;
        case UTF8_GETCHR(u8"以"): return 1071;
        case UTF8_GETCHR(u8"血"): return 1075;
        case UTF8_GETCHR(u8"复"): return 1079;
        case UTF8_GETCHR(u8"活"): return 1083;
        case UTF8_GETCHR(u8"欢"): return 1087;
        case UTF8_GETCHR(u8"迎"): return 1091;
        case UTF8_GETCHR(u8"临"): return 1095;
        case UTF8_GETCHR(u8"本"): return 1099;
        case UTF8_GETCHR(u8"飞"): return 1103;
        case UTF8_GETCHR(u8"船"): return 1107;
        case UTF8_GETCHR(u8"对"): return 1111;
        case UTF8_GETCHR(u8"那"): return 1115;
        case UTF8_GETCHR(u8"些"): return 1119;
        case UTF8_GETCHR(u8"灾"): return 1123;
        case UTF8_GETCHR(u8"难"): return 1127;
        case UTF8_GETCHR(u8"去"): return 1131;
        case UTF8_GETCHR(u8"纪"): return 1135;
        case UTF8_GETCHR(u8"念"): return 1139;
        case UTF8_GETCHR(u8"。"): return 1143;
        case UTF8_GETCHR(u8"个"): return 1147;
        case UTF8_GETCHR(u8"区"): return 1151;
        case UTF8_GETCHR(u8"年"): return 1155;
        case UTF8_GETCHR(u8"占"): return 1159;
        case UTF8_GETCHR(u8"领"): return 1163;
        case UTF8_GETCHR(u8"封"): return 1167;
        case UTF8_GETCHR(u8"日"): return 1171;
        case UTF8_GETCHR(u8"期"): return 1175;
        case UTF8_GETCHR(u8"啊"): return 1179;
        case UTF8_GETCHR(u8"事"): return 1183;
        case UTF8_GETCHR(u8"是"): return 1187;
        case UTF8_GETCHR(u8"前"): return 1191;
        case UTF8_GETCHR(u8"应"): return 1195;
        case UTF8_GETCHR(u8"急"): return 1199;
        case UTF8_GETCHR(u8"装"): return 1203;
        case UTF8_GETCHR(u8"级"): return 1207;
        case UTF8_GETCHR(u8"解"): return 1211;
        case UTF8_GETCHR(u8"击"): return 1215;
        case UTF8_GETCHR(u8"杀"): return 1219;
        case UTF8_GETCHR(u8"手"): return 1223;
        case UTF8_GETCHR(u8"枪"): return 1227;
        case UTF8_GETCHR(u8"危"): return 1231;
        case UTF8_GETCHR(u8"险"): return 1235;
        case UTF8_GETCHR(u8"来"): return 1239;
        case UTF8_GETCHR(u8"袭"): return 1243;
        case UTF8_GETCHR(u8"认"): return 1247;
        case UTF8_GETCHR(u8"路"): return 1251;
        case UTF8_GETCHR(u8"题"): return 1255;
        // 1259 intentional break
        case UTF8_GETCHR(u8"问"): return 1263;
        case UTF8_GETCHR(u8"通"): return 1267;
        case UTF8_GETCHR(u8"许"): return 1271;
        case UTF8_GETCHR(u8"多"): return 1275;
        case UTF8_GETCHR(u8"其"): return 1279;
        case UTF8_GETCHR(u8"他"): return 1283;
        case UTF8_GETCHR(u8"计"): return 1287;
        case UTF8_GETCHR(u8"算"): return 1291;
        // case UTF8_GETCHR(u8"到"): return 1295; // re-use
        // case UTF8_GETCHR(u8"达"): return 1299; // re-use
        case UTF8_GETCHR(u8"径"): return 1303;
        case UTF8_GETCHR(u8"万"): return 1307;
        case UTF8_GETCHR(u8"建"): return 1311;
        case UTF8_GETCHR(u8"造"): return 1315;
        case UTF8_GETCHR(u8"环"): return 1319;
        case UTF8_GETCHR(u8"绕"): return 1323;
        case UTF8_GETCHR(u8"球"): return 1327;
        case UTF8_GETCHR(u8"和"): return 1331;
        case UTF8_GETCHR(u8"轨"): return 1335;
        case UTF8_GETCHR(u8"由"): return 1339;
        case UTF8_GETCHR(u8"于"): return 1343;
        case UTF8_GETCHR(u8"运"): return 1347;
        case UTF8_GETCHR(u8"行"): return 1351;
        case UTF8_GETCHR(u8"断"): return 1355;
        case UTF8_GETCHR(u8"重"): return 1359;
        case UTF8_GETCHR(u8"永"): return 1363;
        case UTF8_GETCHR(u8"远"): return 1367;
        case UTF8_GETCHR(u8"两"): return 1371;
        case UTF8_GETCHR(u8"访"): return 1375;
        case UTF8_GETCHR(u8"预"): return 1379;
        case UTF8_GETCHR(u8"性"): return 1383;
        // case UTF8_GETCHR(u8"传"): return 1387; re-use
        case UTF8_GETCHR(u8"称"): return 1391;
        case UTF8_GETCHR(u8"冒"): return 1395;
        case UTF8_GETCHR(u8"—"): return 1399;

        case UTF8_GETCHR(u8"遇"): return 1403;
        case UTF8_GETCHR(u8"奇"): return 1407;
        case UTF8_GETCHR(u8"怪"): return 1411;
        case UTF8_GETCHR(u8"但"): return 1415;
        case UTF8_GETCHR(u8"老"): return 1419;
        case UTF8_GETCHR(u8"板"): return 1423;
        case UTF8_GETCHR(u8"向"): return 1427;
        case UTF8_GETCHR(u8"证"): return 1431;
        case UTF8_GETCHR(u8"从"): return 1435;
        case UTF8_GETCHR(u8"今"): return 1439;
        case UTF8_GETCHR(u8"后"): return 1443;
        case UTF8_GETCHR(u8"再"): return 1447;
        case UTF8_GETCHR(u8"自"): return 1451;
        case UTF8_GETCHR(u8"讯"): return 1455;
        case UTF8_GETCHR(u8"就"): return 1459;
        case UTF8_GETCHR(u8"毛"): return 1463;
        case UTF8_GETCHR(u8"病"): return 1467;
        case UTF8_GETCHR(u8"暴"): return 1471;

        case UTF8_GETCHR(u8"当"): return 1475;
        case UTF8_GETCHR(u8"火"): return 1479;
        case UTF8_GETCHR(u8"很"): return 1483;
        case UTF8_GETCHR(u8"幸"): return 1487;
        case UTF8_GETCHR(u8"较"): return 1491;
        case UTF8_GETCHR(u8"躲"): return 1495;
        case UTF8_GETCHR(u8"非"): return 1499;
        case UTF8_GETCHR(u8"搜"): return 1503;
        case UTF8_GETCHR(u8"避"): return 1507;
        case UTF8_GETCHR(u8"逃"): return 1511;
        case UTF8_GETCHR(u8"越"): return 1515;
        case UTF8_GETCHR(u8"走"): return 1519;
        case UTF8_GETCHR(u8"似"): return 1523;
        case UTF8_GETCHR(u8"乎"): return 1527;
        case UTF8_GETCHR(u8"严"): return 1531;
        case UTF8_GETCHR(u8"倒"): return 1535;
        case UTF8_GETCHR(u8"霉"): return 1539;
        case UTF8_GETCHR(u8"全"): return 1543;
        case UTF8_GETCHR(u8"视"): return 1547;
        case UTF8_GETCHR(u8"挡"): return 1551;
        case UTF8_GETCHR(u8"着"): return 1555;

        case UTF8_GETCHR(u8"影"): return 1559;
        case UTF8_GETCHR(u8"响"): return 1563;
        case UTF8_GETCHR(u8"特"): return 1567;
        case UTF8_GETCHR(u8"别"): return 1571;
        case UTF8_GETCHR(u8"站"): return 1575;
        case UTF8_GETCHR(u8"观"): return 1579;
        case UTF8_GETCHR(u8"察"): return 1583;
        case UTF8_GETCHR(u8"收"): return 1587;
        case UTF8_GETCHR(u8"力"): return 1591;
        case UTF8_GETCHR(u8"西"): return 1595;
        case UTF8_GETCHR(u8"切"): return 1599;
        case UTF8_GETCHR(u8"激"): return 1603;
        case UTF8_GETCHR(u8"增"): return 1607;
        case UTF8_GETCHR(u8"候"): return 1611;
        case UTF8_GETCHR(u8"东"): return 1615;
        case UTF8_GETCHR(u8"透"): return 1619;
        case UTF8_GETCHR(u8"降"): return 1623;
        case UTF8_GETCHR(u8"落"): return 1627;
        case UTF8_GETCHR(u8"遮"): return 1631;
        case UTF8_GETCHR(u8"掩"): return 1635;
        case UTF8_GETCHR(u8"瞥"): return 1639;
        case UTF8_GETCHR(u8"披"): return 1643;
        case UTF8_GETCHR(u8"斗"): return 1647;
        case UTF8_GETCHR(u8"篷"): return 1651;
        case UTF8_GETCHR(u8"想"): return 1655;
        case UTF8_GETCHR(u8"也"): return 1659;
        case UTF8_GETCHR(u8"该"): return 1663;
        case UTF8_GETCHR(u8"而"): return 1667;
        case UTF8_GETCHR(u8"看"): return 1671;

        case UTF8_GETCHR(u8"现"): return 1675;
        case UTF8_GETCHR(u8"基"): return 1679;
        case UTF8_GETCHR(u8"恒"): return 1683;
        case UTF8_GETCHR(u8"把"): return 1687;
        case UTF8_GETCHR(u8"消"): return 1691;
        case UTF8_GETCHR(u8"知"): return 1695;
        case UTF8_GETCHR(u8"久"): return 1699;
        case UTF8_GETCHR(u8"才"): return 1703;
        case UTF8_GETCHR(u8"理"): return 1707;
        case UTF8_GETCHR(u8"己"): return 1711;
        case UTF8_GETCHR(u8"所"): return 1715;
        case UTF8_GETCHR(u8"派"): return 1719;
        case UTF8_GETCHR(u8"士"): return 1723;
        case UTF8_GETCHR(u8"兵"): return 1727;
        case UTF8_GETCHR(u8"刚"): return 1731;
        case UTF8_GETCHR(u8"明"): return 1735;
        case UTF8_GETCHR(u8"号"): return 1739;
        case UTF8_GETCHR(u8"它"): return 1743;
        case UTF8_GETCHR(u8"暂"): return 1747;
        case UTF8_GETCHR(u8"免"): return 1751;
        // Intentional gap
        case UTF8_GETCHR(u8"家"): return 1803;
        case UTF8_GETCHR(u8"集"): return 1820;
        case UTF8_GETCHR(u8"度"): return 1824;
        case UTF8_GETCHR(u8"键"): return 1838;
        case UTF8_GETCHR(u8"合"): return 1842;
        case UTF8_GETCHR(u8"角"): return 1846;
        case UTF8_GETCHR(u8"低"): return 1850;
        case UTF8_GETCHR(u8"比"): return 1854;
        case UTF8_GETCHR(u8"简"): return 1858;
        case UTF8_GETCHR(u8"普"): return 1862;
        case UTF8_GETCHR(u8"困"): return 1866;
        case UTF8_GETCHR(u8"显"): return 1870;
        case UTF8_GETCHR(u8"示"): return 1874;
        case UTF8_GETCHR(u8"钟"): return 1878;
        case UTF8_GETCHR(u8"启"): return 1882;
        case UTF8_GETCHR(u8"震"): return 1886;
        case UTF8_GETCHR(u8"夜"): return 1890;
        case UTF8_GETCHR(u8"模"): return 1894;
        case UTF8_GETCHR(u8"式"): return 1898;
        case UTF8_GETCHR(u8"固"): return 1918;
        case UTF8_GETCHR(u8"相"): return 1922;
        case UTF8_GETCHR(u8"坏"): return 1926;
        case UTF8_GETCHR(u8"愁"): return 1930;
        case UTF8_GETCHR(u8"哦"): return 1934;
        case UTF8_GETCHR(u8"玩"): return 1938;
        case UTF8_GETCHR(u8"意"): return 1942;
        case UTF8_GETCHR(u8"压"): return 1946;
        case UTF8_GETCHR(u8"注"): return 1950;
        case UTF8_GETCHR(u8"距"): return 1954;
        case UTF8_GETCHR(u8"伤"): return 1958;
        case UTF8_GETCHR(u8"巨"): return 1962;
        case UTF8_GETCHR(u8"起"): return 1966;
        case UTF8_GETCHR(u8"像"): return 1970;
        case UTF8_GETCHR(u8"旧"): return 1974;
        case UTF8_GETCHR(u8"怀"): return 1978;
        case UTF8_GETCHR(u8"表"): return 1982;
        case UTF8_GETCHR(u8"常"): return 1986;
        case UTF8_GETCHR(u8"错"): return 1990;
        case UTF8_GETCHR(u8"金"): return 1994;
        case UTF8_GETCHR(u8"属"): return 1998;
        case UTF8_GETCHR(u8"值"): return 2002;
        case UTF8_GETCHR(u8"菲"): return 2006;
        case UTF8_GETCHR(u8"备"): return 2010;
        case UTF8_GETCHR(u8"指"): return 2014;
        case UTF8_GETCHR(u8"方"): return 2018;
        case UTF8_GETCHR(u8"小"): return 2022;
        case UTF8_GETCHR(u8"心"): return 2026;
        case UTF8_GETCHR(u8"稳"): return 2030;
        case UTF8_GETCHR(u8"射"): return 2034;
        case UTF8_GETCHR(u8"尝"): return 2038;
        case UTF8_GETCHR(u8"试"): return 2042;
        case UTF8_GETCHR(u8"附"): return 2046;
        case UTF8_GETCHR(u8"做"): return 2050;
        case UTF8_GETCHR(u8"让"): return 2054;
        case UTF8_GETCHR(u8"卫"): return 2058;
        case UTF8_GETCHR(u8"狼"): return 2062;
        case UTF8_GETCHR(u8"修"): return 2066;

        case UTF8_GETCHR(u8"变"): return 2071;
        case UTF8_GETCHR(u8"弄"): return 2075;
        case UTF8_GETCHR(u8"几"): return 2079;
        case UTF8_GETCHR(u8"斤"): return 2083;
        case UTF8_GETCHR(u8"结"): return 2087;
        case UTF8_GETCHR(u8"束"): return 2091;

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


Platform::TextureCpMapper locale_doublesize_texture_map()
{
    return doublesize_texture_map;
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

    if (str_cmp(lang_name, "chinese") == 0) {
        // Yeah, this is lazy. I could write a string to
        // number-to-unicode-string algorithm for chinese, but I don't feel like
        // it right now.
        switch (num) {
        default:
        case 1:
            return "一";
        case 2:
            return "二";
        case 3:
            return "三";
        case 4:
            return "四";
        case 5:
            return "五";
        case 6:
            return "六";
        case 7:
            return "七";
        case 8:
            return "八";
        case 9:
            return "九";
        case 10:
            return "十";
        case 11:
            return "十一";
        case 12:
            return "十二";
        case 13:
            return "十三";
        case 14:
            return "十四";
        case 15:
            return "十五";
        case 16:
            return "十六";
        case 17:
            return "十七";
        case 18:
            return "十八";
        case 19:
            return "十九";
        case 20:
            return "二十";
        case 21:
            return "二十一";
        case 22:
            return "二十二";
        case 23:
            return "二十三";
        case 24:
            return "二十四";
        case 25:
            return "二十五";
        case 26:
            return "二十六";
        case 27:
            return "二十七";
        case 28:
            return "二十八";
        case 29:
            return "二十九";
        case 30:
            return "三十";
        case 31:
            return "三十一";
        case 32:
            return "三十二";
        case 33:
            return "三十三";
        case 34:
            return "三十四";
        case 35:
            return "三十五";
        case 36:
            return "三十六";
        case 37:
            return "三十七";
        case 38:
            return "三十八";
        case 39:
            return "三十九";
        case 40:
            return "四十";
        case 41:
            return "四十一";
        case 42:
            return "四十二";
        case 43:
            return "四十三";
        case 44:
            return "四十四";
        case 45:
            return "四十五";
        case 46:
            return "四十六";
        case 47:
            return "四十七";
        case 48:
            return "四十八";
        case 49:
            return "四十九";
        }
    } else {
        // Arabic numerals
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


LocalizedText locale_localized_language_name(Platform& pfrm, int language)
{
    const auto cached_lang = ::language_id;

    ::language_id = language;

    auto ret = locale_string(pfrm, LocaleString::language_name);

    ::language_id = cached_lang;

    return ret;
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
                    pfrm.fatal("null byte in localized text");
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
        pfrm.fatal("missing strings file for language");
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
