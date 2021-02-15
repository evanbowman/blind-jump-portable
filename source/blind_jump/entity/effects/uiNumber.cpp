#include "uiNumber.hpp"
#include "blind_jump/game.hpp"


UINumber::UINumber(const Vec2<Float>& position, s8 val, Id parent)
    : timer_(0), hold_(0), parent_(parent)
{
    set_position({position.x - 16, position.y - 12});

    // We only store enough sprites for two place values. Anyway, given the
    // use-case for this class, displaying damage info, we should never
    // really need more than two digits.

    set_val(val);
}


void UINumber::set_val(s8 val)
{
    if (val > 99) {
        val = 99;
    }

    if (val < -99) {
        val = -99;
    }

    sign_.set_position(position_);

    sign_.set_size(Sprite::Size::w16_h32);
    extra_.set_size(Sprite::Size::w16_h32);
    sprite_.set_size(Sprite::Size::w16_h32);

    if (val < 0) {
        sign_.set_texture_index(52);
    } else {
        sign_.set_texture_index(53);
    }


    if (val / 10) {
        extra_.set_texture_index(42 + abs(val) / 10);
        extra_.set_position({position_.x + 9, position_.y});
        sprite_.set_texture_index(42 + abs(val) % 10);
        sprite_.set_position({position_.x + 19, position_.y});
    } else {
        sprite_.set_alpha(Sprite::Alpha::transparent);
        extra_.set_texture_index(42 + abs(val));
        extra_.set_position({position_.x + 9, position_.y});
    }
}


s8 UINumber::get_val() const
{
    s8 result = 0;

    if (sprite_.get_alpha() == Sprite::Alpha::transparent) {
        result = 42 - extra_.get_texture_index();
    } else {
        result = 42 - sprite_.get_texture_index();
        result += (42 - extra_.get_texture_index()) * 10;
    }

    if (sign_.get_texture_index() == 52) {
        result = -result;
    }

    return result;
}


void UINumber::update(Platform& pf, Game& game, Microseconds dt)
{
    timer_ += dt;

    static const float MOVEMENT_RATE_CONSTANT = 0.000054f;

    if (timer_ < milliseconds(280)) {
        offset_ -= MOVEMENT_RATE_CONSTANT * dt;
    } else {
        // for (auto& entity : game.effects().get<UINumber>()) {
        //     if (entity.get() not_eq this and
        //         manhattan_length(entity->get_position(), position_) < 4 and
        //         entity->alive()) {

        //         auto val = entity->get_val();
        //         val += this->get_val();

        //         entity->set_val(val);
        //         entity->hold();

        //         this->kill();
        //     }
        // }
    }

    if (auto e = get_entity_by_id(game, parent_)) {
        set_position({e->get_position().x - 16, e->get_position().y - 12});
    }

    sign_.set_position({position_.x, position_.y + offset_});
    extra_.set_position({position_.x + 9, position_.y + offset_});
    sprite_.set_position({position_.x + 19, position_.y + offset_});

    if (timer_ > milliseconds(750)) {
        if (hold_) {
            hold_ -= dt;
        }

        if (hold_ <= 0) {
            this->kill();
        }
    }
}
