#include "player.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"
#include "wallCollision.hpp"


// NOTE: The player code was ported over from the original version of
// BlindJump from 2016. This code is kind of a mess.


static const Vec2<s32> v_origin{8, 16};
static const Vec2<s32> h_origin{16, 16};
// static const Player::Health initial_health{3};


Entity::Health Player::initial_health(Platform& pfrm) const
{
    return 3;
}


Player::Player(Platform& pfrm)
    : Entity(initial_health(pfrm)), frame_(0),
      frame_base_(ResourceLoc::player_still_down), anim_timer_(0),
      invulnerability_timer_(0), l_speed_(0.f), r_speed_(0.f), u_speed_(0.f),
      d_speed_(0.f), hitbox_{&position_, {{10, 22}, {9, 14}}}
{
    sprite_.set_position({104.f, 64.f});
    sprite_.set_size(Sprite::Size::w16_h32);
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
        // If 'injured' sound not playing, play sound...
    }
}


void Player::on_collision(Platform& pf, Game& game, ConglomerateShot&)
{
    if (not Player::is_invulnerable()) {
        game.camera().shake();
    }

    Player::injured(pf, game, Health(1));
}


void Player::on_collision(Platform& pf, Game& game, OrbShot&)
{
    if (not Player::is_invulnerable()) {
        game.camera().shake();
    }

    Player::injured(pf, game, Health(1));
}

void Player::on_collision(Platform& pf, Game& game, WandererBigLaser&)
{
    medium_explosion(pf, game, position_);
    Player::injured(pf, game, Health(2));
}

void Player::on_collision(Platform& pf, Game& game, Wanderer&)
{
    Player::injured(pf, game, Health(1));
}

void Player::on_collision(Platform& pf, Game& game, SnakeHead&)
{
    Player::injured(pf, game, Health(2));
}


void Player::on_collision(Platform& pf, Game& game, Enemy& e)
{
    if (not e.is_allied()) {
        Player::injured(pf, game, Health(1));
    }
}


void Player::on_collision(Platform& pf, Game& game, Twin& t)
{
    Player::injured(pf, game, Health(1));
}


void Player::on_collision(Platform& pf, Game& game, Drone& drone)
{
    if (not drone.is_allied()) {
        Player::injured(pf,
                        game,
                        drone.state() == Drone::State::rush ? Health(2)
                                                            : Health(1));
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


void Player::on_collision(Platform& pf, Game& game, Item& item)
{
    if (not item.ready()) {
        return;
    }
    switch (item.get_type()) {
    case Item::Type::heart:
        sprite_.set_mix({ColorConstant::spanish_crimson, 255});
        add_health(1);
        pf.speaker().play_sound("pop", 1);
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
    fade_color_anim_.advance(sprite_, dt);
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
    static_assert(player_still_left == 22 and player_still_right == 29 and
                  player_still_up == 11 and player_still_down == 0);

    const auto t_idx = sprite_.get_texture_index();
    if (t_idx >= player_walk_down and t_idx < player_walk_up) {
        sprite_.set_texture_index(player_still_down);
    }
    if (t_idx >= player_walk_up and t_idx <= player_still_up) {
        sprite_.set_texture_index(player_still_up);
    }
    if (t_idx >= player_walk_left and t_idx < player_still_left) {
        sprite_.set_texture_index(player_still_left);
    }
    if (t_idx >= player_walk_right and t_idx < player_still_right) {
        sprite_.set_texture_index(player_still_right);
    }
}


void Player::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& input = pfrm.keyboard();
    const bool up = input.pressed<Key::up>();
    const bool down = input.pressed<Key::down>();
    const bool left = input.pressed<Key::left>();
    const bool right = input.pressed<Key::right>();
    const bool shoot = input.pressed(game.action1_key());

    const auto wc = check_wall_collisions(game.tiles(), *this);

    soft_update(pfrm, game, dt);

    if (not shoot) {
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
            down, left, right, shoot);
    }
    if (input.up_transition<Key::down>()) {
        on_key_released<ResourceLoc::player_still_down, 0>(
            up, left, right, shoot);
    }
    if (input.up_transition<Key::left>()) {
        on_key_released<ResourceLoc::player_still_left, 0>(
            up, down, right, shoot);
    }
    if (input.up_transition<Key::right>()) {
        on_key_released<ResourceLoc::player_still_right, 0>(
            up, down, left, shoot);
    }

    const auto frame_persist = [&] {
        if (shoot) {
            return 130000;
        } else {
            return 100000;
        }
    }();

    switch (frame_base_) {
    case ResourceLoc::player_still_up:
    case ResourceLoc::player_still_down:
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_texture_index(frame_base_);
        sprite_.set_origin(v_origin);
        break;

    case ResourceLoc::player_still_left:
    case ResourceLoc::player_still_right:
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_texture_index(frame_base_);
        sprite_.set_origin(h_origin);
        break;

    case ResourceLoc::player_walk_up:
        update_animation<1>(pfrm, dt, 9, frame_persist);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin(v_origin);
        break;

    case ResourceLoc::player_walk_down:
        update_animation<1>(pfrm, dt, 9, frame_persist);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin(v_origin);
        break;

    case ResourceLoc::player_walk_left:
        update_animation<2>(pfrm, dt, 5, frame_persist);
        sprite_.set_texture_index(frame_base_ + frame_);
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin(h_origin);
        break;

    case ResourceLoc::player_walk_right:
        update_animation<2>(pfrm, dt, 5, frame_persist);
        sprite_.set_texture_index(frame_base_ + frame_);
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin(h_origin);
        break;

    default:
        break;
    }

    static const float MOVEMENT_RATE_CONSTANT = 0.000054f;

    if (external_force_) {
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

    Vec2<Float> new_pos{
        position_.x - ((l_speed_ + -r_speed_) * dt * MOVEMENT_RATE_CONSTANT),
        position_.y - ((u_speed_ + -d_speed_) * dt * MOVEMENT_RATE_CONSTANT)};
    Entity::set_position(new_pos);
    sprite_.set_position(new_pos);
    shadow_.set_position(new_pos);

    blaster_.update(pfrm, game, dt, [this] {
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
    }());

    if (shoot) {
        blaster_.set_visible(true);
        blaster_.shoot(pfrm, game);
    } else {
        blaster_.set_visible(false);
    }
}


void Player::set_visible(bool visible)
{
    blaster_.set_visible(visible);

    if (visible) {
        sprite_.set_alpha(Sprite::Alpha::opaque);
        shadow_.set_alpha(Sprite::Alpha::translucent);
    } else {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        shadow_.set_alpha(Sprite::Alpha::transparent);
    }
}


void Player::move(const Vec2<Float>& pos)
{
    Player::set_position(pos);
    sprite_.set_position(pos);
    shadow_.set_position(pos);
    frame_base_ = ResourceLoc::player_still_down;
    sprite_.set_texture_index(frame_base_);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin(v_origin);
    external_force_.reset();
}


Blaster::Blaster()
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
                p->parameter_ += 1;
            }
        };

        if (auto p = get_powerup(game, Powerup::Type::accelerator)) {
            max_lasers = 3;
            reload_interval = milliseconds(150);
            applied_powerups.push_back(p);
        }

        if (auto p = get_powerup(game, Powerup::Type::explosive_rounds)) {
            expl_rounds = p->parameter_;
            applied_powerups.push_back(p);
        }


        if (int(length(game.effects().get<Laser>())) < max_lasers) {

            for (auto p : applied_powerups) {
                p->parameter_ -= 1;
                p->dirty_ = true;
            }

            reload_ = reload_interval;

            pf.speaker().play_sound("blaster", 4);

            if (not[&] {
                    if (expl_rounds > 0) {
                        game.camera().shake();
                        medium_explosion(pf, game, position_);

                        return game.effects().spawn<Laser>(
                            position_, dir_, Laser::Mode::explosive);
                    } else {
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
