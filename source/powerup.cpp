#include "powerup.hpp"
#include "game.hpp"


void add_powerup(Game& game,
                 Powerup::Type t,
                 int param,
                 Powerup::DisplayMode mode)
{
    // Collapse with existing powerup
    for (auto& powerup : game.powerups()) {
        if (powerup.type_ == t) {
            powerup.parameter_ += param;
            powerup.dirty_ = true;
            return;
        }
    }

    if (game.powerups().full()) {
        game.powerups().erase(game.powerups().begin());
    }

    game.powerups().push_back({t, param, false, mode});
}


Powerup* get_powerup(Game& game, Powerup::Type t)
{
    for (auto& powerup : game.powerups()) {
        if (powerup.type_ == t) {
            return &powerup;
        }
    }
    return nullptr;
}
