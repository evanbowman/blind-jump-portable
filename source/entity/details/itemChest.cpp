#include "itemChest.hpp"
#include "game.hpp"
#include "platform/platform.hpp"
#include "state.hpp"
#include "string.hpp"


ItemChest::ItemChest(const Vec2<Float>& pos, Item::Type item)
    : state_(State::closed), item_(item)
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
                        push_notification(
                            pfrm, game, [enemies_remaining, &pfrm](Text& text) {
                                static const auto prefix = "locked, ";
                                static const auto suffix1 = " enemies left";
                                static const auto suffix2 = " enemy left";

                                const char* suffix = [&] {
                                    if (enemies_remaining == 1) {
                                        return suffix2;
                                    } else {
                                        return suffix1;
                                    }
                                }();

                                const auto width =
                                    str_len(prefix) +
                                    integer_text_length(enemies_remaining) +
                                    str_len(suffix);

                                const auto margin =
                                    centered_text_margins(pfrm, width);

                                left_text_margin(text, margin);

                                text.append(prefix);
                                text.append(enemies_remaining);
                                text.append(suffix);

                                right_text_margin(text, margin);
                            });

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
