#pragma once


#include "number/numeric.hpp"
#include <memory>


class Platform;
class Game;
class State;


using StatePtr = std::unique_ptr<State, void(*)(State*)>;


class State {
public:
    virtual void enter(Platform&, Game&){};
    virtual void exit(Platform&, Game&){};

    // Returns a new state, if we're transitioning to another state, otherwise,
    // if the next state will be the same state, returns an empty state
    // pointer.


    virtual StatePtr update(Platform& platform, Game& game, Microseconds delta) = 0;

    virtual ~State()
    {
    }

    static StatePtr initial();
};
