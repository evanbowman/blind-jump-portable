#include "conf.hpp"


static bool is_whitespace(char c)
{
    return c == ' ' or c == '\n' or c == '\r' or c == '\t';
}


// This may be the laziest garbage code that I've ever written. It's basically a
// non-conforming ini file parser. There are libraries out there, but they all
// use too much memory for my purposes. This parser uses minimal stack space.
StringBuffer<31> get_conf(const char* pos, const char* section, const char* key)
{
    const int section_len = str_len(section);
    const int key_len = str_len(key);

    enum State { seek_section,
                 match_section,
                 seek_key,
                 read_value } state = State::seek_section;

    StringBuffer<31> result;

#define EAT_LINE()                              \
    while (true) {                              \
        if (*pos == '\0') {                     \
            return result;                      \
        }                                       \
        if (*pos == '\n') {                     \
            goto TOP;                           \
        }                                       \
        ++pos;                                  \
    }                                           \

    while (*pos not_eq '\0') {
    TOP:
        switch (state) {
        case State::seek_section:
            if (*pos == '#') {
                EAT_LINE();
            }
            if (*pos == '[') {
                state = State::match_section;
            }
            ++pos;
            break;

        case State::match_section: {
            for (int i = 0; i < section_len; ++i) {
                if (pos[i] == '\0') {
                    return result;
                }
                if (pos[i] == ']') {
                    state = State::seek_section;
                    pos += i;
                    goto TOP;
                }
                if (pos[i] not_eq section[i]) {
                    state = State::seek_section;
                    goto TOP;
                }
            }
            pos += section_len;
            while (true) {
                if (*pos == '\0') {
                    return result;
                }
                if (*pos == ']') {
                    ++pos;
                    break;
                } else if (not is_whitespace(*pos)) {
                    state = State::seek_section;
                }
                ++pos;
            }
            state = State::seek_key;
            break;
        }

        case State::seek_key: {
            while (true) {
                if (*pos == '\0' or *pos == '[') {
                    return result;
                }
                if (*pos == '#') {
                    EAT_LINE();
                }
                if (not is_whitespace(*pos)) {
                    break;
                }
                ++pos;
            }
            for (int i = 0; i < key_len; ++i) {
                if (pos[i] == '\0') {
                    return result;
                }
                if (key[i] not_eq pos[i]) {
                    while (true) {
                        if (*pos == '\0') {
                            return result;
                        }
                        if (*pos == '\n') {
                            state = State::seek_key;
                            ++pos;
                            goto TOP;
                        }
                        ++pos;
                    }
                }
            }
            pos += key_len;
            while (true) {
                if (*pos == '\0') {
                    return result;
                }
                if (*pos == '=') {
                    ++pos;
                    break;
                } else if (not is_whitespace(*pos)) {
                    while (true) {
                        if (*pos == '\0') {
                            return result;
                        }
                        if (*pos == '\n') {
                            state = State::seek_key;
                            ++pos;
                            goto TOP;
                        }
                        ++pos;
                    }
                }
                ++pos;
            }
            state = State::read_value;
            break;
        }

        case State::read_value: {
            while (true) {
                if (*pos == '\0' or *pos == '#') {
                    return result;
                }
                if (is_whitespace(*pos)) {
                    return result;
                }
                result.push_back(*pos);
                ++pos;
            }
            break;
        }
    }
    }
    return result;
}
