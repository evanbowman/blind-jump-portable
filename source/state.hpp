#pragma once


#include "number/numeric.hpp"


class Platform;
class Game;


class State {
public:
    virtual State* update(Platform& platform,
                          Microseconds delta,
                          Game& game) = 0;
    virtual ~State() {}

    static State* initial();
};
