#pragma once

#include "screen.hpp"
#include "keyboard.hpp"


class Platform {
public:

    Platform();

    Screen& screen();

    Keyboard& keyboard();

private:
    Screen screen_;
    Keyboard keyboard_;
};
