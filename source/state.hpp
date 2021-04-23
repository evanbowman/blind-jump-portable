#pragma once


#include "graphics/overlay.hpp"
#include "number/numeric.hpp"
#include "string.hpp"
#include <memory>


class Platform;
class Game;
class State;


using StatePtr = std::unique_ptr<State, void (*)(State*)>;
using DeferredState = Function<16, StatePtr()>;

class State {
public:
    virtual void enter(Platform&, Game&, State& prev_state);
    virtual void exit(Platform&, Game&, State& next_state);

    // Returns a new state, if we're transitioning to another state, otherwise,
    // if the next state will be the same state, returns an empty state
    // pointer.

    virtual StatePtr update(Platform& platform, Game& game, Microseconds delta);

    State()
    {
    }

    State(const State&) = delete;

    virtual ~State()
    {
    }

    static StatePtr initial(Platform&, Game&);
};


StatePtr null_state();


using NotificationStr = StringBuffer<70>;
void push_notification(Platform& pfrm,
                       State* state,
                       const NotificationStr& string);


// Yeah, this breaks encapsulation. But this is an edge case, where the boss
// needs to display its own health, but due to state changes outside of an
// individual entity's control, it doesn't make sense for the enemy itself to
// own the GUI's health bar.
void show_boss_health(Platform& pfrm,
                      Game& game,
                      int health_bar,
                      Float percentage);

void hide_boss_health(Game& game);
