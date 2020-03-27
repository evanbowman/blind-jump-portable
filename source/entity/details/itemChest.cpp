#include "itemChest.hpp"
#include "game.hpp"
#include "platform/platform.hpp"


ItemChest::ItemChest(const Vec2<Float>& pos) : state_(State::locked)
{
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_position({pos.x, pos.y});
    sprite_.set_origin({8, 16});
    animation_.bind(sprite_);
}


void ItemChest::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& player_pos = game.player().get_position();
    const auto& pos = sprite_.get_position();

    switch (state_) {
    case State::locked:
        break;

    case State::closed:

        fade_color_anim_.advance(sprite_, dt);

        if (sprite_.get_mix().amount_ == 0) {
            sprite_.set_mix({ColorConstant::stil_de_grain, 255});
        }

        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            // You can only open an item chest when you've cleared out all the
            // enemies on the map.
            if (manhattan_length(player_pos, pos) < 32) {
                pfrm.sleep(10);
                state_ = State::opening;
                sprite_.set_mix({ColorConstant::null, 0});
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
            game.score() += 100;
        }
        break;

    case State::opened:
        break;
    }
}
