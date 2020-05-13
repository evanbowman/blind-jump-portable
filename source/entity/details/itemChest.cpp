#include "itemChest.hpp"
#include "game.hpp"
#include "platform/platform.hpp"
#include "state.hpp"
#include "string.hpp"


ItemChest::ItemChest(const Vec2<Float>& pos, Item::Type item)
    : state_(State::closed), item_(item)
{
    position_ = pos;

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_position({pos.x, pos.y});
    sprite_.set_origin({8, 16});
    animation_.bind(sprite_);

    // This is such a hack... but when flipped upside-down, the turret shadow
    // acts as the shadow for item chests, saves some vram.
    shadow_.set_texture_index(TextureMap::turret_shadow);
    shadow_.set_flip({false, true});

    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, 16});
    shadow_.set_position({pos.x, pos.y + 9});
}


void ItemChest::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& player_pos = game.player().get_position();
    const auto& pos = sprite_.get_position();

    switch (state_) {
    case State::closed:

        if (visible()) {
            if (pfrm.keyboard().down_transition<Key::action_2>()) {

                if (manhattan_length(player_pos, pos) < 32) {

                    int enemies_remaining = 0;
                    game.enemies().transform([&](auto& buf) {
                        using T =
                            typename std::remove_reference<decltype(buf)>::type;

                        using VT = typename T::ValueType::element_type;

                        // SnakeBody and SnakeTail are technically not
                        // enemies, we only want to count each snake head
                        // segment.
                        if (not std::is_same<VT, SnakeBody>() and
                            not std::is_same<VT, SnakeTail>()) {

                            enemies_remaining += length(buf);
                        }
                    });

                    if (enemies_remaining) {

                        NotificationStr str;

                        str += "locked, ";

                        std::array<char, 40> buffer;
                        to_string(enemies_remaining, buffer.data(), 10);

                        str += buffer.data();
                        str += [&] {
                            if (enemies_remaining == 1) {
                                return " enemy left";
                            } else {
                                return " enemies left";
                            }
                        }();
                        ;

                        push_notification(pfrm, game, str);

                    } else {
                        pfrm.sleep(10);
                        state_ = State::opening;

                        pfrm.speaker().play_sound("creak", 1);
                    }
                }
            }
        }

        // fade_color_anim_.advance(sprite_, dt);

        // if (sprite_.get_mix().amount_ == 0) {
        //     sprite_.set_mix({ColorConstant::stil_de_grain, 255});
        // }
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
            game.inventory().push_item(pfrm, game, item_);
            game.score() += 100;
        }
        break;

    case State::opened:
        break;
    }
}
