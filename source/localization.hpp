#pragma once


#include "blind_jump/localeString.hpp"
#include "bulkAllocator.hpp"
#include "dateTime.hpp"
#include "string.hpp"


void locale_set_language(int language_id);
void locale_set_language_english();
int locale_get_language();

bool locale_requires_doublesize_font();

using LocalizedStrBuffer = StringBuffer<1987>;
using LocalizedText = DynamicMemory<LocalizedStrBuffer>;
LocalizedText locale_string(Platform& pfrm, LocaleString ls);


StringBuffer<31> locale_language_name(int language);

LocalizedText locale_localized_language_name(Platform& pfrm, int language);


// In most cases, you do not want to call this function directly, better to call
// the localized version, locale_num2str. Only call english__to_string for
// logging purposes, where the language is assumed to be english.
void english__to_string(int num, char* buffer, int base);


template <u32 length> StringBuffer<length> to_string(int num)
{
    char temp[length];
    english__to_string(num, temp, 10);

    return temp;
}


void locale_num2str(int num, char* buffer, int base);


template <u32 buffer_size>
void format_time(StringBuffer<buffer_size>& str, const DateTime& dt)
{
    char buffer[48];

    locale_num2str(dt.date_.month_, buffer, 10);
    str += buffer;
    str += "/";

    locale_num2str(dt.date_.day_, buffer, 10);
    str += buffer;
    str += "/";

    locale_num2str(dt.date_.year_, buffer, 10);
    str += buffer;
    str += " ";

    locale_num2str(dt.hour_, buffer, 10);
    str += buffer;
    str += ":";

    locale_num2str(dt.minute_, buffer, 10);
    str += buffer;
    str += ":";

    locale_num2str(dt.second_, buffer, 10);
    str += buffer;
}


template <u32 buffer_size>
void log_format_time(StringBuffer<buffer_size>& str, const DateTime& dt)
{
    const auto saved_language = locale_get_language();
    locale_set_language(1);

    format_time(str, dt);

    locale_set_language(saved_language);
}
