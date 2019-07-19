#include "itemChest.hpp"
#include "game.hpp"
#include "platform.hpp"


ItemChest::ItemChest(const Vec2<Float>& pos) : state_(State::closed)
{
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_position({pos.x, pos.y});
    sprite_.set_origin({8, 16});
    animation_.bind(sprite_);
}


void ItemChest::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& player_pos = game.get_player().get_position();
    const auto& pos = sprite_.get_position();
    switch (state_) {
    case State::closed:
        if (pfrm.keyboard().down_transition<Keyboard::Key::action_2>()) {
            if (manhattan_length(player_pos, pos) < 32) {
                pfrm.sleep(10);
                state_ = State::opening;
            }
        }
        break;

    case State::opening:
        animation_.advance(sprite_, dt);
        if (animation_.done(sprite_)) {
            state_ = State::settle;
        }
        break;

    case State::settle:
        if (animation_.reverse(sprite_, dt)) {
            state_ = State::opened;
        }
        break;

    case State::opened:
        break;
    }
}
