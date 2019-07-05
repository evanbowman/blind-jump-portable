#include "itemChest.hpp"
#include "game.hpp"
#include "platform.hpp"


ItemChest::ItemChest() : state_(State::closed)
{
    sprite_.set_position({50.f, 50.f});
    sprite_.set_texture_index(26);
}


void ItemChest::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& player_pos = game.get_player().get_position();
    const auto& pos = sprite_.get_position();
    switch (state_) {
    case State::closed:
        if (pfrm.keyboard().down_transition<Keyboard::Key::action_2>()) {
            if (manhattan_length(player_pos, pos) < 32) {
                state_ = State::opening;
            }
        }
        break;

    case State::opening:
        animation_.advance(sprite_, dt);
        if (animation_.done()) {
            state_ = State::opened;
        }
        break;

    case State::opened:
        break;
    }
}
