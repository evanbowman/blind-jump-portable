#include "theFirstExplorer.hpp"
#include "entity/effects/explosion.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include "wallCollision.hpp"


static const char* boss_music = "sb_omega";


static const Entity::Health initial_health = 75;


TheFirstExplorer::TheFirstExplorer(const Vec2<Float>& position)
    : Entity(initial_health), hitbox_{&position_, {16, 38}, {8, 24}}, timer_(0),
      timer2_(0)
{
    set_position(position);

    sprite_.set_texture_index(12);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});

    head_.set_texture_index(13);
    head_.set_size(Sprite::Size::w16_h32);
    head_.set_origin({8, 32});

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position({position_.x, position_.y - 16});
    shadow_.set_position(position);
}


static bool wall_in_path(const Vec2<Float>& direction,
                         const Vec2<Float> position,
                         Game& game,
                         const Vec2<Float>& destination)
{
    int dist = abs(distance(position, destination));

    for (int i = 24; i < dist; i += 16) {
        Vec2<Float> pos = {position.x + direction.x * i,
                           position.y + direction.y * i};

        auto tile_coord = to_tile_coord(pos.cast<s32>());

        if (not is_walkable(
                game.tiles().get_tile(tile_coord.x, tile_coord.y))) {
            return true;
        }
    }

    return false;
}


void TheFirstExplorer::update(Platform& pf, Game& game, Microseconds dt)
{
    // auto second_form = [&] {
    //                        return get_health() < (initial_health + 10 ) / 2;
    //                    };

    auto face_left = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_right();
            } else {
                face_left();
            }
        };

    auto to_wide_sprite = [&] {
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin({16, 16});
        head_.set_size(Sprite::Size::w32_h32);
        head_.set_origin({16, 32});
    };

    auto to_narrow_sprite = [&] {
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin({8, 16});
        head_.set_size(Sprite::Size::w16_h32);
        head_.set_origin({8, 32});
    };

    static const auto dash_duration = milliseconds(650);
    const float movement_rate = 0.00003f;

    switch (state_) {
    case State::sleep:
        if (visible()) {
            timer_ += dt;
            if (timer_ > seconds(1)) {
                state_ = State::draw_weapon;
                timer_ = 0;

                game.play_music(pf, boss_music, seconds(235));
            }
        }
        break;

    case State::still: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;

            auto t_coord = to_tile_coord(position_.cast<s32>());
            const auto tile = game.tiles().get_tile(t_coord.x, t_coord.y);

            if (random_choice<2>() and not is_border(tile)) {
                state_ = State::draw_weapon;

                to_wide_sprite();

                sprite_.set_texture_index(7);
                head_.set_texture_index(41);

            } else {
                state_ = State::prep_dash;
            }
        }
        break;
    }

    case State::draw_weapon: {
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = sprite_.get_texture_index();
            if (index < 12) {
                sprite_.set_texture_index(index + 1);
                head_.set_texture_index(
                    std::min(u32(45), head_.get_texture_index() + 1));

            } else {
                state_ = State::shooting;
                timer_ = 0;
            }
        }

        break;
    }

    case State::shooting:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(2000)) {
            timer_ = 0;
            state_ = State::prep_dash;

            to_narrow_sprite();

            sprite_.set_texture_index(12);
            head_.set_texture_index(13);
        }

        break;

    case State::prep_dash:
        timer_ += dt;
        if (timer_ > milliseconds(200)) {
            timer_ = 0;
            timer2_ = 0;

            to_wide_sprite();

            sprite_.set_texture_index(13);
            head_.set_texture_index(46);

            state_ = State::dash;

            s16 dir;
            Vec2<Float> dest;
            Vec2<Float> unit;
            do {
                dir = ((static_cast<float>(random_choice<359>())) / 360) *
                      INT16_MAX;

                unit = {(float(cosine(dir)) / INT16_MAX),
                        (float(sine(dir)) / INT16_MAX)};
                speed_ = 5.f * unit;

                if (random_choice<3>() == 0) {
                    unit = direction(position_, game.player().get_position());
                    speed_ = 5.f * unit;
                }

                const auto tolerance = milliseconds(70);
                dest = position_ +
                       speed_ * ((dash_duration + tolerance) * movement_rate);

            } while (abs(speed_.x) < 1 // The dashing animation just looks
                                       // strange when the character is moving
                                       // vertically.
                     or wall_in_path(unit, position_, game, dest));
        }
        break;

    case State::dash: {
        timer_ += dt;
        timer2_ += dt;

        if (timer_ > milliseconds(100)) {
            timer_ = 0;

            if (sprite_.get_texture_index() == 13) {
                sprite_.set_texture_index(14);
                head_.set_texture_index(47);
            } else {
                sprite_.set_texture_index(13);
                head_.set_texture_index(46);
            }
        }

        auto transition = [&] {
            state_ = State::still;

            timer_ = 0;
            timer2_ = 0;

            to_narrow_sprite();

            sprite_.set_texture_index(12);
            head_.set_texture_index(13);
        };

        if (timer2_ > dash_duration) {
            transition();
        }

        if (speed_.x < 0) {
            face_left();
        } else {
            face_right();
        }

        position_.x += speed_.x * dt * movement_rate;
        position_.y += speed_.y * dt * movement_rate;

        sprite_.set_position(position_);
        head_.set_position({position_.x, position_.y - 16});
        shadow_.set_position(position_);

        break;
    }
    }

    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void TheFirstExplorer::on_collision(Platform& pf, Game& game, Laser&)
{
    debit_health(1);

    sprite_.set_mix({ColorConstant::aerospace_orange, 255});
    head_.set_mix({ColorConstant::aerospace_orange, 255});

    if (not alive()) {

        big_explosion(pf, game, position_);

        auto neg = random_choice<2>();
        const auto off = neg ? -56.f : 56.f;
        big_explosion(pf, game, {position_.x - off, position_.y - off});
        big_explosion(pf, game, {position_.x + off, position_.y + off});


        game.stop_music(pf);

        game.transporter().set_position(position_);
    }
}
