#pragma once


#include "number/numeric.hpp"


class Platform;
class Game;


class State {
public:
    virtual void enter(Platform&, Game&){};

    virtual State*
    update(Platform& platform, Game& game, Microseconds delta) = 0;

    virtual ~State()
    {
    }

    static State* initial();
};
