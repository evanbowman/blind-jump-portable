#pragma once

#include "platform.hpp"
#include "deltaClock.hpp"


class Game {
public:
    Game();

    void update(Platform& platform, Microseconds delta);

private:
    Sprite test_;
};
