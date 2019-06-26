#pragma once

#include "platform.hpp"


class Game;


class Probe {
public:
    void update(const Platform&, Game&, Microseconds);
};
