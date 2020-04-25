#include "gatekeeper.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include "boss.hpp"


static const Entity::Health initial_health = 100;


Gatekeeper::Gatekeeper(const Vec2<Float>& position) :
    Enemy(initial_health, position, {{32, 50}, {16, 36}}),
    state_(State::idle),
    timer_(0)
{
    sprite_.set_texture_index(7);
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    head_.set_texture_index(6);
    head_.set_size(Sprite::Size::w32_h32);
    head_.set_origin({16, 48});

    shadow_.set_texture_index(41);
    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_origin({16, 9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position(position);
    shadow_.set_position(position);
}


void Gatekeeper::update(Platform& pfrm, Game& game, Microseconds dt)
{
    static constexpr const Microseconds jump_duration = milliseconds(500);
    static constexpr const Float movement_rate = 0.000033f;

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
                face_left();
            } else {
                face_right();
            }
        };

    switch (state_) {
    case State::idle:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(400)) {
            timer_ = 0;

            if (distance(position_, game.player().get_position()) > 100 or
                random_choice<4>() == 0) {
                state_ = State::jump;

                sprite_.set_texture_index(48);
                head_.set_texture_index(42);

            } else {
                // TODO: attack...

            }
        }
        break;

    case State::jump:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index < 46) {
                head_.set_texture_index(index + 1);
                sprite_.set_texture_index(sprite_.get_texture_index() + 1);

            } else {
                state_ = State::airborne;

                Vec2<Float> dest;
                Vec2<Float> unit;

                int tries = 0;

                do {

                    if (tries++ > 0) {
                        const s16 dir = ((static_cast<float>(random_choice<359>())) / 360) *
                            INT16_MAX;

                        unit = {(float(cosine(dir)) / INT16_MAX),
                                (float(sine(dir)) / INT16_MAX)};

                    } else {
                        unit = direction(position_, game.player().get_position());

                    }

                    move_vec_ = 5.f * unit;

                    const auto tolerance = milliseconds(90);
                    dest = position_ +
                        move_vec_ * ((jump_duration + tolerance) * movement_rate);

                } while (wall_in_path(unit, position_, game, dest));

            }
        }
        break;

    case State::airborne: {
        timer_ += dt;

        const Float offset =
                10 * Float(sine(4 * 3.14 * 0.0027f * timer_ + 180)) /
                std::numeric_limits<s16>::max();

        position_.x += move_vec_.x * dt * movement_rate;
        position_.y += move_vec_.y * dt * movement_rate;

        sprite_.set_position({position_.x, position_.y - abs(offset)});
        head_.set_position({position_.x, position_.y - abs(offset)});
        shadow_.set_position(position_);

        if (timer_ > milliseconds(300)) {
            sprite_.set_texture_index(53);
            head_.set_texture_index(47);
        }
        if (timer_ > jump_duration) {
            head_.set_position(position_);
            sprite_.set_position(position_);

            head_.set_texture_index(45);
            sprite_.set_texture_index(45 + 6);
            state_ = State::landing;
        }
        break;
    }

    case State::landing:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index > 42) {
                head_.set_texture_index(index - 1);
                sprite_.set_texture_index(sprite_.get_texture_index() - 1);

            } else {
                state_ = State::idle;
                sprite_.set_texture_index(7);
                head_.set_texture_index(6);

            }
        }
        break;
    }


    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, Laser&)
{
    debit_health(1);

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});

    show_boss_health(pfrm, game, Float(get_health()) / initial_health);
}


void Gatekeeper::on_death(Platform& pfrm, Game& game)
{
    boss_explosion(pfrm, game, position_);
}
