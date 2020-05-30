#include "conf.hpp"
#include "platform/platform.hpp"


static bool is_whitespace(char c)
{
    return c == ' ' or c == '\n' or c == '\r' or c == '\t';
}


static bool is_ascii_num(char c)
{
    return c > 47 and c < 58;
}


static int conf_atoi(const char* string)
{
    int result = 0;
    unsigned int digit;
    int sign;

    // Our INI parser will only pass in trimmed strings, so no need to skip
    // blanks.
    // /*
    //  * Skip any leading blanks.
    //  */
    // while (isspace(*string)) {
    //     string += 1;
    // }

    /*
     * Check for a sign.
     */

    if (*string == '-') {
        sign = 1;
        string += 1;
    } else {
        sign = 0;
        if (*string == '+') {
            string += 1;
        }
    }

    for (;; string += 1) {
        digit = *string - '0';
        if (digit > 9) {
            break;
        }
        result = (10 * result) + digit;
    }

    if (sign) {
        return -result;
    }
    return result;
}


// This may be the laziest garbage code that I've ever written. It's basically a
// non-conforming ini file parser. There are libraries out there, but they all
// use too much memory for my purposes. This parser uses minimal stack space,
// less than 100 bytes. The code is not designed for speed, the priority here is
// small memory footprint.
Conf::String get_conf(const char* data, const char* section, const char* key)
{
    const int section_len = str_len(section);
    const int key_len = str_len(key);

    enum State {
        seek_section,
        match_section,
        seek_key,
        read_value
    } state = State::seek_section;

    Conf::String result;

#define EAT_LINE()                                                             \
    while (true) {                                                             \
        if (*data == '\0') {                                                   \
            return result;                                                     \
        }                                                                      \
        if (*data == '\n') {                                                   \
            goto TOP;                                                          \
        }                                                                      \
        ++data;                                                                \
    }

    while (*data not_eq '\0') {
    TOP:
        switch (state) {
        case State::seek_section:
            if (*data == '#') {
                EAT_LINE();
            }
            if (*data == '[') {
                state = State::match_section;
            }
            ++data;
            break;

        case State::match_section: {
            for (int i = 0; i < section_len; ++i) {
                if (data[i] == '\0') {
                    return result;
                }
                if (data[i] == ']') {
                    state = State::seek_section;
                    data += i;
                    goto TOP;
                }
                if (data[i] not_eq section[i]) {
                    state = State::seek_section;
                    goto TOP;
                }
            }
            data += section_len;
            while (true) {
                if (*data == '\0') {
                    return result;
                }
                if (*data == ']') {
                    ++data;
                    break;
                } else if (not is_whitespace(*data)) {
                    state = State::seek_section;
                }
                ++data;
            }
            state = State::seek_key;
            break;
        }

        case State::seek_key: {
            while (true) {
                if (*data == '\0' or *data == '[') {
                    return result;
                }
                if (*data == '#') {
                    EAT_LINE();
                }
                if (not is_whitespace(*data)) {
                    break;
                }
                ++data;
            }
            for (int i = 0; i < key_len; ++i) {
                if (data[i] == '\0') {
                    return result;
                }
                if (key[i] not_eq data[i]) {
                    while (true) {
                        if (*data == '\0') {
                            return result;
                        }
                        if (*data == '\n') {
                            state = State::seek_key;
                            ++data;
                            goto TOP;
                        }
                        ++data;
                    }
                }
            }
            data += key_len;
            while (true) {
                if (*data == '\0') {
                    return result;
                }
                if (*data == '=') {
                    ++data;
                    break;
                } else if (not is_whitespace(*data)) {
                    while (true) {
                        if (*data == '\0') {
                            return result;
                        }
                        if (*data == '\n') {
                            state = State::seek_key;
                            ++data;
                            goto TOP;
                        }
                        ++data;
                    }
                }
                ++data;
            }
            state = State::read_value;
            break;
        }

        case State::read_value: {
            while (true) {
                if (*data == '\0' or *data == '#') {
                    return result;
                }
                if (is_whitespace(*data)) {
                    return result;
                }
                result.push_back(*data);
                ++data;
            }
            break;
        }
        }
    }
    return result;
}


Conf::Value Conf::get(const char* section, const char* key)
{
    auto buf = get_conf(pfrm_.config_data(), section, key);

    if (buf.empty()) {
        return {};
    }

    const bool is_numeric = [&buf] {
        for (char c : buf) {
            if (not is_ascii_num(c)) {
                return false;
            }
        }
        return true;
    }();

    if (is_numeric) {
        return conf_atoi(buf.c_str());
    } else {
        return buf;
    }
}


void Conf::fatal(const char* section, const char* key)
{
    StringBuffer<64> err;
    err += "conf: ";
    err += section;
    err += key;

    error(pfrm_, err.c_str());

    while (true)
        ;
}
