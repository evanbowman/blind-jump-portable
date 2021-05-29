#include "player.hpp"
#include "blind_jump/game.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"
#include "wallCollision.hpp"


// NOTE: The player code was ported over from the original version of
// BlindJump from 2016. This code is kind of a mess.


// static const Vec2<s16> v_origin{8, 16};
static const Vec2<s16> h_origin{16, 16};
// static const Player::Health initial_health{3};


static const auto dodge_flicker_light_color = custom_color(0xadffff);
static const auto dodge_flicker_dark_color = custom_color(0x021d96);


Entity::Health Player::initial_health(Platform& pfrm) const
{
    return 3;
}


Player::Player(Platform& pfrm)
    : Entity(initial_health(pfrm)), frame_(0),
      frame_base_(ResourceLoc::player_still_down), anim_timer_(0),
      invulnerability_timer_(0), l_speed_(0.f), r_speed_(0.f), u_speed_(0.f),
      d_speed_(0.f), hitbox_{&position_, {{10, 22}, {9, 14}}},
      dynamic_texture_([&] {
          if (auto t = pfrm.make_dynamic_texture()) {
              return *t;
          } else {
              pfrm.fatal("failed to alloc texture");
          }
      }())
{
    sprite_.set_position({104.f, 64.f});
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({8, 16});
    shadow_.set_origin({8, -9});
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


Vec2<Float> Player::get_speed() const
{
    return {l_speed_ + -r_speed_, u_speed_ + -d_speed_};
}


void Player::revive(Platform& pfrm)
{
    if (not alive()) {
        add_health(initial_health(pfrm));

        invulnerability_timer_ = seconds(1);
        sprite_.set_mix({});
    }
}


void Player::apply_force(const Vec2<Float>& force)
{
    if (external_force_) {
        external_force_ = *external_force_ + force;
    } else {
        external_force_ = force;
    }
}


void Player::injured(Platform& pf, Game& game, Health damage)
{
    if (not Player::is_invulnerable()) {
        pf.sleep(4);
        debit_health(damage);
        sprite_.set_mix({current_zone(game).injury_glow_color_, 255});
        blaster_.get_sprite().set_mix(sprite_.get_mix());
        invulnerability_timer_ = milliseconds(700);

        // if (auto num = game.effects().spawn<UINumber>(
        //         get_position(), damage * -1, id())) {
        //     num->hold(milliseconds(700));
        // }
    }
}


void Player::on_collision(Platform& pf, Game& game, Compactor& compactor)
{
    if (static_cast<int>(compactor.state()) <
        static_cast<int>(Compactor::State::landing)) {
        // If the compactor is still in its falling animation, we do not want to
        // injure th player.
        return;
    }

    if (not Player::is_invulnerable()) {
        game.camera().shake();
    }

    if (game.difficulty() == Settings::Difficulty::easy) {
        Player::injured(pf, game, Health(1));
    } else {
        Player::injured(pf, game, Health(2));
    }

    game.rumble(pf, milliseconds(430));
}


void Player::on_collision(Platform& pf, Game& game, ConglomerateShot&)
{
    if (not Player::is_invulnerable()) {
        game.camera().shake();
    }

    Player::injured(pf, game, Health(1));
    game.rumble(pf, milliseconds(280));
}


void Player::on_collision(Platform& pf, Game& game, OrbShot&)
{
    if (not Player::is_invulnerable()) {
        game.camera().shake();
    }

    Player::injured(pf, game, Health(1));
    game.rumble(pf, milliseconds(280));
}


void Player::on_collision(Platform& pf, Game& game, WandererBigLaser&)
{
    medium_explosion(pf, game, position_);
    Player::injured(pf, game, Health(2));
    game.rumble(pf, milliseconds(390));
}


void Player::on_collision(Platform& pf, Game& game, Wanderer&)
{
    Player::injured(pf, game, Health(1));
    game.rumble(pf, milliseconds(280));
}


void Player::on_collision(Platform& pf, Game& game, SnakeHead&)
{
    if (game.difficulty() == Settings::Difficulty::easy) {
        Player::injured(pf, game, Health(1));
    } else {
        Player::injured(pf, game, Health(2));
    }
    game.rumble(pf, milliseconds(390));
}


void Player::on_collision(Platform& pf, Game& game, Enemy& e)
{
    if (not e.is_allied()) {
        Player::injured(pf, game, Health(1));
        game.rumble(pf, milliseconds(280));
    }
}


void Player::on_collision(Platform& pf, Game& game, Twin& t)
{
    Player::injured(pf, game, Health(1));
    game.rumble(pf, milliseconds(280));
}


void Player::on_collision(Platform& pf, Game& game, Drone& drone)
{
    if (not drone.is_allied()) {
        Player::injured(pf,
                        game,
                        drone.state() == Drone::State::rush and
                                game.difficulty() not_eq
                                    Settings::Difficulty::easy
                            ? (game.rumble(pf, milliseconds(390)), Health(2))
                            : (game.rumble(pf, milliseconds(280)), Health(1)));
    }
}


void Player::on_collision(Platform& pf, Game& game, Theif&)
{
    if (this->is_invulnerable()) {
        return;
    }

    if (get_health() > 1) {
        pf.sleep(8);
        sprite_.set_mix({current_zone(game).injury_glow_color_, 255});
        blaster_.get_sprite().set_mix(sprite_.get_mix());
        invulnerability_timer_ = milliseconds(700);
    }

    while (get_health() > 1) {
        if (game.details().spawn<Item>(position_, pf, Item::Type::heart)) {
            (*game.details().get<Item>().begin())->scatter();
            debit_health(1);
        } else {
            break;
        }
    }
}


void Player::heal(Platform& pfrm, Game& game, Health amount)
{
    sprite_.set_mix({ColorConstant::spanish_crimson, 255});
    add_health(amount);
    pfrm.speaker().play_sound("pop", 1);
    // game.effects().spawn<UINumber>(get_position(), amount, id());
}


void Player::on_collision(Platform& pf, Game& game, Item& item)
{
    if (not item.ready()) {
        return;
    }
    switch (item.get_type()) {
    case Item::Type::heart:
        heal(pf, game, 1);
        break;

    case Item::Type::coin:
        get_score(pf, game, 10);
        break;

    case Item::Type::null:
        break;

    default:
        // TODO
        break;
    }
}


void Player::get_score(Platform& pf, Game& game, u32 score)
{
    sprite_.set_mix({current_zone(game).energy_glow_color_, 255});
    game.score() += score;
    pf.speaker().play_sound("coin", 1);
}


template <Player::ResourceLoc L>
void Player::altKeyResponse(bool k1,
                            bool k2,
                            bool k3,
                            float& speed,
                            bool collision)
{
    if (k1) {
        frame_base_ = L;
        if (not collision) {
            if (k2 or k3) {
                speed = 0.83f;
            } else {
                speed = 1.24f;
            }
        } else {
            speed = 0.f;
        }
    } else {
        speed = 0.f;
    }
}


template <Player::ResourceLoc L>
void Player::key_response(bool k1,
                          bool k2,
                          bool k3,
                          bool k4,
                          float& speed,
                          bool collision)
{
    if (k1) {
        if (not k2 and not k3 and not k4 and frame_base_ != L) {
            frame_base_ = L;
        }
        if (not collision) {
            if (k3 or k4) {
                speed = 1.00f;
            } else {
                speed = 1.50f;
            }
        } else {
            speed = 0.f;
        }
    } else {
        speed = 0.f;
    }
}


template <Player::ResourceLoc S, uint8_t maxIndx>
void Player::on_key_released(bool k2, bool k3, bool k4, bool x)
{
    if (not k2 && not k3 && not k4) {
        if (not x) {
            frame_base_ = S;
            frame_ = maxIndx;
        } else {
            switch (frame_base_) {
            case Player::ResourceLoc::player_walk_down:
                frame_base_ = Player::ResourceLoc::player_still_down;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_up:
                frame_base_ = Player::ResourceLoc::player_still_up;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_left:
                frame_base_ = Player::ResourceLoc::player_still_left;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_right:
                frame_base_ = Player::ResourceLoc::player_still_right;
                frame_ = 0;
                break;

            default:
                break;
            }
        }
    }
}


static uint8_t remap_vframe(uint8_t index)
{
    switch (index) {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 2;
    case 3:
        return 1;
    case 4:
        return 0;
    case 5:
        return 3;
    case 6:
        return 4;
    case 7:
        return 4;
    case 8:
        return 3;
    case 9:
        return 0;
    default:
        return 0;
    }
}


template <u8 StepSize>
void Player::update_animation(Platform& pf,
                              Microseconds dt,
                              u8 max_index,
                              Microseconds count)
{
    anim_timer_ += dt;
    if (anim_timer_ > count) {
        anim_timer_ -= count;
        frame_ += 1;

        switch (max_index) {
        case 9:
            if (frame_ == 3 or frame_ == 6) {
                footstep(pf);
            }
            break;

        case 5:
            if (frame_ == 2) {
                footstep(pf);
            }
            break;

        default:
            error(pf, "logic error in code");
            break;
        }
    }
    if (frame_ > max_index) {
        frame_ = 0;
    }
}


void Player::soft_update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (sprite_.get_mix().color_ not_eq dodge_flicker_light_color and
        sprite_.get_mix().color_ not_eq dodge_flicker_dark_color) {

        fade_color_anim_.advance(sprite_, dt);
    }

    blaster_.get_sprite().set_mix(sprite_.get_mix());

    if (invulnerability_timer_ > 0) {
        if (invulnerability_timer_ > dt) {
            invulnerability_timer_ -= dt;
        } else {
            invulnerability_timer_ = 0;
        }
    }

    // Because we're comparing ranges below, make sure the someone hasn't
    // re-ordered the tiles.
    // static_assert(player_still_left == 22 and player_still_right == 29 and
    //               player_still_up == 11 and player_still_down == 0);

    const auto t_idx = sprite_.get_texture_index();
    if (t_idx >= player_walk_down and t_idx < player_walk_up) {
        set_sprite_texture(player_still_down);
    }
    if (t_idx >= player_walk_up and t_idx <= player_still_up) {
        set_sprite_texture(player_still_up);
    }
    if (t_idx >= player_walk_left and t_idx < player_still_left) {
        set_sprite_texture(player_still_left);
    }
    if (t_idx >= player_walk_right and t_idx < player_still_right) {
        set_sprite_texture(player_still_right);
    }
}


Cardinal Player::facing() const
{
    switch (frame_base_) {
    case ResourceLoc::player_walk_left:
    case ResourceLoc::player_still_left:
        return Cardinal::west;
    case ResourceLoc::player_walk_right:
    case ResourceLoc::player_still_right:
        return Cardinal::east;
    case ResourceLoc::player_walk_up:
    case ResourceLoc::player_still_up:
        return Cardinal::north;
    case ResourceLoc::player_walk_down:
    case ResourceLoc::player_still_down:
        return Cardinal::south;
    default:
        return Cardinal::south;
    }
}


void Player::push_dodge_effect(Platform& pfrm, Game& game)
{
    if (auto dt = pfrm.make_dynamic_texture()) {
        if (game.effects().spawn<DynamicEffect>(
                rng::sample<2>(Vec2<Float>{position_.x, position_.y},
                               rng::utility_state),
                *dt,
                milliseconds(100),
                sprite_.get_texture_index(),
                0)) {
            (*game.effects().get<DynamicEffect>().begin())
                ->sprite()
                .set_mix({dodge_flicker_light_color, 255});
            (*game.effects().get<DynamicEffect>().begin())->set_backdrop(true);
        }
    }
}


void Player::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& input = pfrm.keyboard();
    const bool up = input.pressed<Key::up>();
    const bool down = input.pressed<Key::down>();
    const bool left = input.pressed<Key::left>();
    const bool right = input.pressed<Key::right>();
    const bool strafe = [&] {
        switch (game.persistent_data().settings_.button_mode_) {
        case Settings::ButtonMode::count:
        case Settings::ButtonMode::strafe_separate:
            return input.pressed(game.action2_key());

        case Settings::ButtonMode::strafe_combined:
            break;
        }
        return input.pressed(game.action1_key());
    }();


    auto wc = check_wall_collisions(game.tiles(), *this);

    int collision_count = 0;
    if (wc.up) {
        ++collision_count;
    }
    if (wc.down) {
        ++collision_count;
    }
    if (wc.left) {
        ++collision_count;
    }
    if (wc.right) {
        ++collision_count;
    }

    if (UNLIKELY(collision_count > 2)) {
        // How did this happen? We've managed to get ourselves stuck inside a
        // wall. This shouldn't happen under normal circumstances, but what if
        // the game lags, resulting in a large delta time? So we should handle
        // this scenario, however unlikely.
        set_position(last_pos_);
    }

    soft_update(pfrm, game, dt);

    switch (state_) {
    case State::normal: {
        if (not strafe) {
            key_response<ResourceLoc::player_walk_up>(
                up, down, left, right, u_speed_, wc.up);
            key_response<ResourceLoc::player_walk_down>(
                down, up, left, right, d_speed_, wc.down);
            key_response<ResourceLoc::player_walk_left>(
                left, right, down, up, l_speed_, wc.left);
            key_response<ResourceLoc::player_walk_right>(
                right, left, down, up, r_speed_, wc.right);
        } else {
            if (frame_base_ == ResourceLoc::player_walk_up or
                frame_base_ == ResourceLoc::player_still_up) {

                altKeyResponse<ResourceLoc::player_walk_up>(
                    up, left, right, u_speed_, wc.up);
                altKeyResponse<ResourceLoc::player_walk_up>(
                    down, left, right, d_speed_, wc.down);
                altKeyResponse<ResourceLoc::player_walk_up>(
                    left, up, down, l_speed_, wc.left);
                altKeyResponse<ResourceLoc::player_walk_up>(
                    right, up, down, r_speed_, wc.right);

            } else if (frame_base_ == ResourceLoc::player_walk_down or
                       frame_base_ == ResourceLoc::player_still_down) {

                altKeyResponse<ResourceLoc::player_walk_down>(
                    up, left, right, u_speed_, wc.up);
                altKeyResponse<ResourceLoc::player_walk_down>(
                    down, left, right, d_speed_, wc.down);
                altKeyResponse<ResourceLoc::player_walk_down>(
                    left, up, down, l_speed_, wc.left);
                altKeyResponse<ResourceLoc::player_walk_down>(
                    right, up, down, r_speed_, wc.right);

            } else if (frame_base_ == ResourceLoc::player_walk_right or
                       frame_base_ == ResourceLoc::player_still_right) {

                altKeyResponse<ResourceLoc::player_walk_right>(
                    up, left, right, u_speed_, wc.up);
                altKeyResponse<ResourceLoc::player_walk_right>(
                    down, left, right, d_speed_, wc.down);
                altKeyResponse<ResourceLoc::player_walk_right>(
                    left, up, down, l_speed_, wc.left);
                altKeyResponse<ResourceLoc::player_walk_right>(
                    right, up, down, r_speed_, wc.right);

            } else if (frame_base_ == ResourceLoc::player_walk_left or
                       frame_base_ == ResourceLoc::player_still_left) {

                altKeyResponse<ResourceLoc::player_walk_left>(
                    up, left, right, u_speed_, wc.up);
                altKeyResponse<ResourceLoc::player_walk_left>(
                    down, left, right, d_speed_, wc.down);
                altKeyResponse<ResourceLoc::player_walk_left>(
                    left, up, down, l_speed_, wc.left);
                altKeyResponse<ResourceLoc::player_walk_left>(
                    right, up, down, r_speed_, wc.right);
            }
        }

        if (input.up_transition<Key::up>()) {
            on_key_released<ResourceLoc::player_still_up, 0>(
                down, left, right, strafe);
        }
        if (input.up_transition<Key::down>()) {
            on_key_released<ResourceLoc::player_still_down, 0>(
                up, left, right, strafe);
        }
        if (input.up_transition<Key::left>()) {
            on_key_released<ResourceLoc::player_still_left, 0>(
                up, down, right, strafe);
        }
        if (input.up_transition<Key::right>()) {
            on_key_released<ResourceLoc::player_still_right, 0>(
                up, down, left, strafe);
        }

        const auto frame_persist = [&] {
            if (strafe) {
                return 130000;
            } else {
                return 100000;
            }
        }();

        switch (frame_base_) {
        case ResourceLoc::player_still_up:
        case ResourceLoc::player_still_down:
            sprite_.set_size(Sprite::Size::w32_h32);
            // sprite_.set_size(Sprite::Size::w16_h32);
            set_sprite_texture(frame_base_);
            // sprite_.set_origin(v_origin);
            sprite_.set_origin(h_origin);
            break;

        case ResourceLoc::player_still_left:
        case ResourceLoc::player_still_right:
            sprite_.set_size(Sprite::Size::w32_h32);
            set_sprite_texture(frame_base_);
            sprite_.set_origin(h_origin);
            break;

        case ResourceLoc::player_walk_up:
            update_animation<1>(pfrm, dt, 9, frame_persist);
            set_sprite_texture(frame_base_ + remap_vframe(frame_));
            sprite_.set_size(Sprite::Size::w32_h32);
            // sprite_.set_size(Sprite::Size::w16_h32);
            // sprite_.set_origin(v_origin);
            sprite_.set_origin(h_origin);
            break;

        case ResourceLoc::player_walk_down:
            update_animation<1>(pfrm, dt, 9, frame_persist);
            set_sprite_texture(frame_base_ + remap_vframe(frame_));
            sprite_.set_size(Sprite::Size::w32_h32);
            // sprite_.set_size(Sprite::Size::w16_h32);
            // sprite_.set_origin(v_origin);
            sprite_.set_origin(h_origin);
            break;

        case ResourceLoc::player_walk_left:
            update_animation<2>(pfrm, dt, 5, frame_persist);
            set_sprite_texture(frame_base_ + frame_);
            sprite_.set_size(Sprite::Size::w32_h32);
            sprite_.set_origin(h_origin);
            break;

        case ResourceLoc::player_walk_right:
            update_animation<2>(pfrm, dt, 5, frame_persist);
            set_sprite_texture(frame_base_ + frame_);
            sprite_.set_size(Sprite::Size::w32_h32);
            sprite_.set_origin(h_origin);
            break;

        default:
            break;
        }


        if (not input.pressed(game.action1_key())) {
            dodge_timer_ += dt;
        }
        if (dodge_timer_ >= [&] {
                switch (game.persistent_data().settings_.difficulty_) {
                case Settings::Difficulty::easy:
                    return milliseconds(500);
                case Settings::Difficulty::count:
                case Settings::Difficulty::normal:
                    break;
                case Settings::Difficulty::hard:
                case Settings::Difficulty::survival:
                    return seconds(2);
                }
                return seconds(1); // + milliseconds(500);
            }()) {
            dodge_timer_ = 0;
            dodges_ = std::min(dodges_ + 1, max_dodges);
        }

        if (((game.persistent_data().settings_.button_mode_ ==
                  Settings::ButtonMode::strafe_combined and
              pfrm.keyboard().down_transition(game.action2_key())) or
             (game.persistent_data().settings_.button_mode_ ==
                  Settings::ButtonMode::strafe_separate and
              pfrm.keyboard().down_transition<Key::alt_1>())) and
            (left or right or up or down) and
            length(game.effects().get<DialogBubble>()) == 0 and dodges_ > 0) {
            dodges_ -= 1;
            game.rumble(pfrm, milliseconds(150));
            state_ = State::pre_dodge;


            if (auto dt = pfrm.make_dynamic_texture()) {
                if (game.effects().spawn<DynamicEffect>(
                        rng::sample<2>(
                            Vec2<Float>{position_.x, position_.y + 14},
                            rng::utility_state),
                        *dt,
                        milliseconds(90),
                        104,
                        4)) {
                    (*game.effects().get<DynamicEffect>().begin())
                        ->sprite()
                        .set_mix({custom_color(0xf9f7f6), 255});

                    (*game.effects().get<DynamicEffect>().begin())
                        ->set_backdrop(true);
                }
            }

            push_dodge_effect(pfrm, game);

            dodge_timer_ = 0;
            l_speed_ *= 3;
            r_speed_ *= 3;
            u_speed_ *= 3;
            d_speed_ *= 3;
            pfrm.sleep(9);
            game.camera().shake(8);
            pfrm.speaker().play_sound("dodge", 1);
            //sprite_.set_mix({current_zone(game).energy_glow_color_, 255});
            sprite_.set_mix({dodge_flicker_light_color, 255});
            blaster_.set_visible(false);
            weapon_hide_timer_ = 0;

            switch (facing()) {
            case Cardinal::west:
                if (l_speed_ > r_speed_) {
                    set_sprite_texture(98);
                } else {
                    set_sprite_texture(102);
                }
                break;

            case Cardinal::east:
                if (r_speed_ > l_speed_) {
                    set_sprite_texture(99);
                } else {
                    set_sprite_texture(103);
                }
                break;

            case Cardinal::north:
                set_sprite_texture(101);
                break;

            case Cardinal::south:
                set_sprite_texture(100);
                break;
            }
#if defined(__GBA__) or defined(__PSP__)
            game.effects().spawn<Particle>(position_,
                                           dodge_flicker_light_color,
                                           0.0000538f,
                                           false,
                                           milliseconds(500));
            game.effects().spawn<Particle>(position_,
                                           dodge_flicker_light_color,
                                           0.0000538f,
                                           false,
                                           milliseconds(500));
#endif // __GBA__ or __PSP__
        }
        break;
    }

    case State::pre_dodge:
        weapon_hide_timer_ += dt;
        if (weapon_hide_timer_ > milliseconds(50)) {
            weapon_hide_timer_ = 0;
            push_dodge_effect(pfrm, game);
        }

        dodge_timer_ += dt;
        if (dodge_timer_ > milliseconds(80)) {
            state_ = State::dodge;
            sprite_.set_mix({dodge_flicker_dark_color, 255});
        }
        break;

    case State::dodge:

        weapon_hide_timer_ += dt;
        if (weapon_hide_timer_ > milliseconds(80)) {
            weapon_hide_timer_ = 0;
            push_dodge_effect(pfrm, game);
        }

        dodge_timer_ += dt;

        if (dodge_timer_ > milliseconds(80)) {
            // sprite_.set_mix({custom_color(0x99ffff), 75});
            sprite_.set_mix({});
        }
        if (wc.up) {
            u_speed_ = 0;
        }
        if (wc.down) {
            d_speed_ = 0;
        }
        if (wc.left) {
            l_speed_ = 0;
        }
        if (wc.right) {
            r_speed_ = 0;
        }
        if (l_speed_) {
            l_speed_ = interpolate(0.f, l_speed_, 0.000000175f * dt);
        }
        if (r_speed_) {
            r_speed_ = interpolate(0.f, r_speed_, 0.000000175f * dt);
        }
        if (u_speed_) {
            u_speed_ = interpolate(0.f, u_speed_, 0.000000175f * dt);
        }
        if (d_speed_) {
            d_speed_ = interpolate(0.f, d_speed_, 0.000000175f * dt);
        }
        if (dodge_timer_ > milliseconds(150)) {
            state_ = State::normal;
            if (not left and not right and not up and not down) {
                switch (frame_base_) {
                case Player::ResourceLoc::player_walk_down:
                    frame_base_ = Player::ResourceLoc::player_still_down;
                    frame_ = 0;
                    break;

                case Player::ResourceLoc::player_walk_up:
                    frame_base_ = Player::ResourceLoc::player_still_up;
                    frame_ = 0;
                    break;

                case Player::ResourceLoc::player_walk_left:
                    frame_base_ = Player::ResourceLoc::player_still_left;
                    frame_ = 0;
                    break;

                case Player::ResourceLoc::player_walk_right:
                    frame_base_ = Player::ResourceLoc::player_still_right;
                    frame_ = 0;
                    break;

                default:
                    break;
                }
            }
        }
        break;
    }

    static const float MOVEMENT_RATE_CONSTANT = 0.000054f;

    if (external_force_ and state_ == State::normal) {
        if (external_force_->x > 0) {
            if (not wc.right) {
                r_speed_ += external_force_->x;
            }
        } else {
            if (not wc.left) {
                l_speed_ += abs(external_force_->x);
            }
        }
        if (external_force_->y > 0) {
            if (not wc.down) {
                d_speed_ += external_force_->y;
            }
        } else {
            if (not wc.up) {
                u_speed_ += abs(external_force_->y);
            }
        }
        external_force_.reset();
    }

    const Vec2<Float> move_vec = {
        (l_speed_ + -r_speed_) * MOVEMENT_RATE_CONSTANT,
        (u_speed_ + -d_speed_) * MOVEMENT_RATE_CONSTANT};

    last_pos_ = get_position();

    Vec2<Float> new_pos{position_.x - (move_vec.x * dt),
                        position_.y - (move_vec.y * dt)};
    Entity::set_position(new_pos);
    sprite_.set_position(new_pos);
    shadow_.set_position(new_pos);

    blaster_.update(pfrm, game, dt, facing());

    switch (game.persistent_data().settings_.button_mode_) {
    case Settings::ButtonMode::strafe_separate:
        if (input.pressed(game.action1_key())) {
            blaster_.set_visible(true);
            blaster_.shoot(pfrm, game);
            weapon_hide_timer_ = seconds(1);
        }

        if (weapon_hide_timer_ > 0) {
            weapon_hide_timer_ -= dt;

            if (weapon_hide_timer_ <= 0) {
                blaster_.set_visible(false);
            }
        }
        break;

    case Settings::ButtonMode::strafe_combined:
        if (strafe) {
            blaster_.set_visible(true);
            blaster_.shoot(pfrm, game);
        } else {
            blaster_.set_visible(false);
        }
        break;

    case Settings::ButtonMode::count:
        break;
    }
}


void Player::set_visible(bool visible)
{
    if (not visible) {
        blaster_.set_visible(false);
    }

    if (visible) {
        sprite_.set_alpha(Sprite::Alpha::opaque);
        shadow_.set_alpha(Sprite::Alpha::translucent);
    } else {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        shadow_.set_alpha(Sprite::Alpha::transparent);
    }
}


void Player::set_sprite_texture(TextureIndex tidx)
{
    if (tidx not_eq sprite_.get_texture_index()) {
        // Because the player character has a large set of animations, we do not
        // keep all of the frames in vram at once. We need to ask the platform
        // to map our desired frame into video ram.
        if (sprite_.get_size() == Sprite::Size::w32_h32) {
            dynamic_texture_->remap(tidx * 2);
        } else {
            dynamic_texture_->remap(tidx);
        }
    }
    sprite_.set_texture_index(tidx);
}


void Player::init(const Vec2<Float>& pos)
{
    Player::set_position(pos);
    sprite_.set_position(pos);
    shadow_.set_position(pos);
    frame_base_ = ResourceLoc::player_still_down;
    set_sprite_texture(frame_base_);
    // sprite_.set_size(Sprite::Size::w16_h32);
    // sprite_.set_origin(v_origin);
    sprite_.set_origin(h_origin);
    sprite_.set_size(Sprite::Size::w32_h32);
    external_force_.reset();

    state_ = State::normal;
    if (sprite_.get_mix().color_ == dodge_flicker_light_color or
        sprite_.get_mix().color_ == dodge_flicker_dark_color) {
        sprite_.set_mix({});
    }
    dodges_ = 1;
    dodge_timer_ = 0;
}


Blaster::Blaster() : dir_(Cardinal::south), visible_(false)
{
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});
    sprite_.set_texture_index(h_blaster);
}


void Blaster::update(Platform& pf, Game& game, Microseconds dt, Cardinal dir)
{
    const auto player_pos = game.player().get_position();

    dir_ = dir;

    switch (dir) {
    case Cardinal::west:
        sprite_.set_alpha(Sprite::Alpha::opaque);
        sprite_.set_texture_index(h_blaster);
        sprite_.set_flip({true, false});
        position_ = {player_pos.x - 12, player_pos.y + 1};
        break;

    case Cardinal::east:
        sprite_.set_alpha(Sprite::Alpha::opaque);
        sprite_.set_texture_index(h_blaster);
        sprite_.set_flip({false, false});
        position_ = {player_pos.x + 12, player_pos.y + 1};
        break;

    case Cardinal::north:
        sprite_.set_alpha(Sprite::Alpha::transparent);
        position_ = {player_pos.x + 4, player_pos.y - 10};
        break;

    case Cardinal::south:
        sprite_.set_alpha(Sprite::Alpha::opaque);
        sprite_.set_texture_index(v_blaster);
        sprite_.set_flip({false, false});
        position_ = {player_pos.x - 3, player_pos.y + 1};
        break;
    }

    if (not visible_) {
        sprite_.set_alpha(Sprite::Alpha::transparent);
    }

    sprite_.set_position(position_);

    if (reload_ > 0) {
        reload_ -= dt;
    }
}


void Player::footstep(Platform& pf)
{
    switch (rng::choice<4>(rng::utility_state)) {
    case 0:
        pf.speaker().play_sound("footstep1", 0);
        break;

    case 1:
        pf.speaker().play_sound("footstep2", 0);
        break;

    case 2:
        pf.speaker().play_sound("footstep3", 0);
        break;

    case 3:
        pf.speaker().play_sound("footstep4", 0);
        break;
    }
}


void Blaster::shoot(Platform& pf, Game& game)
{
    if (reload_ <= 0) {
        int max_lasers = 2;
        int expl_rounds = 0;
        Microseconds reload_interval = milliseconds(250);

        Buffer<Powerup*, Powerup::max_> applied_powerups;

        auto undo_powerups = [&] {
            for (auto p : applied_powerups) {
                p->parameter_.set(p->parameter_.get() + 1);
            }
        };

        if (auto p = get_powerup(game, Powerup::Type::accelerator)) {
            max_lasers = 3;
            reload_interval = milliseconds(150);
            applied_powerups.push_back(p);
        }

        if (auto p = get_powerup(game, Powerup::Type::explosive_rounds)) {
            expl_rounds = p->parameter_.get();
            applied_powerups.push_back(p);
        }


        if (int(length(game.effects().get<Laser>())) < max_lasers) {

            for (auto p : applied_powerups) {
                p->parameter_.set(p->parameter_.get() - 1);
                p->dirty_ = true;
            }

            reload_ = reload_interval;

            pf.speaker().play_sound("blaster", 4);

            if (not [&] {
                    if (expl_rounds > 0) {
                        game.camera().shake();
                        medium_explosion(pf, game, position_);

                        return game.effects().spawn<Laser>(
                            position_, dir_, Laser::Mode::explosive);
                    } else {
                        game.camera().shake(2);
                        return game.effects().spawn<Laser>(
                            position_, dir_, Laser::Mode::normal);
                    }
                }()) {
                undo_powerups();
                error(pf, "failed to spawn player attack, likely OOM in pool");
            } else {
                net_event::PlayerSpawnLaser e;
                e.dir_ = dir_;
                e.x_.set(position_.cast<s16>().x);
                e.y_.set(position_.cast<s16>().y);

                net_event::transmit(pf, e);
            }
        }
    }
}


void Blaster::set_visible(bool visible)
{
    visible_ = visible;

    if (not visible_) {
        sprite_.set_alpha(Sprite::Alpha::transparent);
    }
}
