#include "itemChest.hpp"
#include "game.hpp"
#include "platform/platform.hpp"
#include "state.hpp"
#include "string.hpp"


ItemChest::ItemChest(const Vec2<Float>& pos, Item::Type item, bool locked)
    : state_(locked ? State::closed_locked : State::closed_unlocked),
      item_(item)
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
    case State::closed_locked:
    case State::closed_unlocked:
        if (visible()) {
            if (pfrm.keyboard().down_transition(game.action2_key())) {

                if (distance(player_pos, pos) < 24) {

                    const int remaining = enemies_remaining(game);
                    if (game.peer() and
                        distance(pos, game.peer()->get_position()) < 32) {

                        push_notification(
                            pfrm,
                            game.state(),
                            locale_string(pfrm,
                                          LocaleString::peer_too_close_to_item)
                                ->c_str());
                    } else if (state_ == State::closed_locked and remaining) {

                        NotificationStr str;

                        str +=
                            locale_string(pfrm, LocaleString::locked)->c_str();

                        std::array<char, 40> buffer;
                        locale_num2str(remaining, buffer.data(), 10);

                        str += buffer.data();
                        str += locale_string(
                                   pfrm,
                                   [&] {
                                       if (remaining == 1) {
                                           return LocaleString::
                                               enemies_remaining_singular;
                                       } else {
                                           return LocaleString::
                                               enemies_remaining_plural;
                                       }
                                   }())
                                   ->c_str();

                        push_notification(pfrm, game.state(), str);

                    } else {
                        if (state_ == State::closed_locked) {
                            // Players can also drop their own item chests in
                            // order to share items between games. Therefore, we
                            // only want to give points to a player for opening
                            // an item chest when that chest was originally
                            // locked, i.e. one of the item chests spawned
                            // during level generation.
                            game.score() += 100;
                        }

                        pfrm.sleep(10);
                        state_ = State::opening;

                        net_event::ItemChestOpened o;
                        o.id_.set(id());
                        net_event::transmit(pfrm, o);

                        pfrm.speaker().play_sound("creak", 1, position_);
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
        }
        break;

    case State::sync_opening:
        animation_.advance(sprite_, dt);
        if (animation_.done(sprite_)) {
            state_ = State::sync_settle;
        }
        break;

    case State::sync_settle:
        if (animation_.reverse(sprite_, dt)) {
            state_ = State::opened;
        }
        break;

    case State::opened:
        break;
    }
}


void ItemChest::sync(Platform& pfrm, const net_event::ItemChestOpened&)
{
    if (state_ == State::closed_locked or state_ == State::closed_unlocked) {
        animation_.bind(sprite_);
        pfrm.speaker().play_sound("creak", 1, position_);
        state_ = State::sync_opening;
    }
}
