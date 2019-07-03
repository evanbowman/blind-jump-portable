#include "player.hpp"


Player::Player() :
    frame_(0),
    state_(State::normal),
    texture_index_(TextureIndex::still_down),
    anim_timer_(0)
{
    sprite_.initialize();
    sprite_.set_position({104.f, 64.f});
}


void Player::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& input = pfrm.keyboard();
    const bool up = input.pressed<Keyboard::Key::up>();
    const bool down = input.pressed<Keyboard::Key::down>();
    const bool left = input.pressed<Keyboard::Key::left>();
    const bool right = input.pressed<Keyboard::Key::right>();

    switch (state_) {
    case State::normal:
        if (up) {
            texture_index_ = TextureIndex::still_up;
        } else if (down) {
            texture_index_ = TextureIndex::still_down;
        } else if (left) {
            texture_index_ = TextureIndex::still_left;
        } else if (right) {
            texture_index_ = TextureIndex::still_right;
        }
        sprite_.set_texture_index(texture_index_);
        break;

    case State::inactive:
        break;
    }
}


const Sprite& Player::get_sprite() const
{
    return sprite_;
}
