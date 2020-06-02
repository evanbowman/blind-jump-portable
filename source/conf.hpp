#pragma once

#include "string.hpp"
#include <variant>


class Platform;


class Conf {
public:
    Conf(Platform& pfrm) : pfrm_(pfrm)
    {
    }

    using Bool = bool;
    using Integer = int;
    using String = StringBuffer<31>;
    using Value = std::variant<std::monostate, Bool, Integer, String>;

    Value get(const char* section, const char* key);

    template <typename T> T expect(const char* section, const char* key)
    {
        const auto v = get(section, key);

        if (auto val = std::get_if<T>(&v)) {
            return *val;
        } else {
            fatal(section, key);
        }
    }

    // While our configuration language doesn't natively support container
    // datatypes, it does offer a library abstraction for lists. If you define a
    // __next parameter in a section, you can use this function to iterate
    // through sections.
    template <typename F>
    void scan_list(const char* start_section, F&& callback)
    {
        Conf::String section;
        section = start_section;

        while (true) {
            callback(section.c_str());

            const auto next = get(section.c_str(), "__next");
            if (auto val = std::get_if<Conf::String>(&next)) {
                section = *val;
            } else {
                return;
            }
        }
    }

private:
    [[noreturn]] void fatal(const char* section, const char* key);

    Platform& pfrm_;
};
