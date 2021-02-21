#include "item.hpp"
#include "blind_jump/game.hpp"
#include "number/random.hpp"
#include "platform/platform.hpp"
#include "wallCollision.hpp"


Item::Item(const Vec2<Float>& pos, Platform&, Type type)
    : state_(State::idle), timer_(rng::get(rng::utility_state)),
      type_(type), hitbox_{&position_, {{10, 10}, {2, 2}}}
{
    position_ = pos - Vec2<Float>{8, 0};
    sprite_.set_position(position_);
    sprite_.set_size(Sprite::Size::w16_h32);

    set_type(type);
}


void Item::on_collision(Platform& pf, Game& game, Player&)
{
    if (not ready()) {
        return;
    }
    Entity::kill();

    auto pos = position_;
    pos.x += 8;

    // FIXME: Sprite scaling is broken on the desktop version of the game.
#if defined(__GBA__) or defined(__PSP__)
    switch (type_) {
    case Type::coin:
        game.effects().spawn<Particle>(pos,
                                       current_zone(game).energy_glow_color_);
        game.effects().spawn<Particle>(pos,
                                       current_zone(game).energy_glow_color_);
        game.effects().spawn<Particle>(pos,
                                       current_zone(game).energy_glow_color_);
        break;

    case Type::heart:
        game.effects().spawn<Particle>(pos, ColorConstant::spanish_crimson);
        game.effects().spawn<Particle>(pos, ColorConstant::spanish_crimson);
        game.effects().spawn<Particle>(pos, ColorConstant::spanish_crimson);
        break;

    default:
        break;
    }
#endif

    pf.sleep(5);
}


void Item::set_type(Type type)
{
    type_ = type;

    switch (type) {
    case Type::heart:
        sprite_.set_texture_index(TextureMap::heart);
        break;

    case Type::coin:
        sprite_.set_texture_index(TextureMap::coin);
        break;

    case Type::null:
        break;

    default:
        // TODO: overworld sprite for all other items
        break;
    }
}


void Item::update(Platform&, Game& game, Microseconds dt)
{
    timer_ += dt;

    if (visible()) {
        if (game.difficulty() == Settings::Difficulty::survival and
            type_ == Type::heart) {
            set_type(Type::coin);
        }

        // Yikes! This is probably expensive...
        const Float offset = 3 *
                             float(sine(4 * 3.14f * 0.001f * timer_ + 180)) /
                             std::numeric_limits<s16>::max();
        sprite_.set_position({position_.x, position_.y + offset});
    }

    if (state_ == State::scatter) {

        constexpr auto duration = seconds(1);

        const auto wc = check_wall_collisions(game.tiles(), *this);
        if (wc.any()) {
            if ((wc.left and step_.x < 0.f) or (wc.right and step_.x > 0.f)) {
                step_.x = -step_.x;
            }
            if ((wc.up and step_.y < 0.f) or (wc.down and step_.y > 0.f)) {
                step_.y = -step_.y;
            }
        }

        position_.x += step_.x * dt;
        position_.y += step_.y * dt;

        if (timer_ > duration) {
            state_ = State::idle;
        }
    }
}


void Item::scatter()
{
    timer_ = 0;
    state_ = State::scatter;
    auto target = rng::sample<64>(position_, rng::utility_state);
    step_ = direction(position_, target) * 0.00013f;
}


bool Item::ready() const
{
    return state_ == State::idle;
}
