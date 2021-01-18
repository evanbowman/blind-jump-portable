#include "signpost.hpp"
#include "blind_jump/state/state_impl.hpp"


Signpost::Signpost(const Vec2<Float>& position, Type type) : type_(type)
{
    position_ = position;

    switch (type_) {
    case Type::knocked_out_peer:
        sprite_.set_origin({16, 0});
        sprite_.set_texture_index(16);
        sprite_.set_position({position.x, position.y - 16});
        break;

    case Type::lander:
        sprite_.set_alpha(Sprite::Alpha::transparent);
        extra_.set_alpha(Sprite::Alpha::transparent);
        break;

    case Type::memorial:
        position_.y -= 22;
        sprite_.set_texture_index(54);
        sprite_.set_position({position_.x, position_.y - 16});
        sprite_.set_origin({16, 32});

        extra_.set_texture_index(55);
        extra_.set_position(position_);
        extra_.set_origin({16, 16});
        break;
    }
}


void Signpost::update(Platform&, Game&, Microseconds dt)
{
    // ...
}


static const LocaleString lander_dialog[] = {LocaleString::lander_str,
                                             LocaleString::empty};


static const LocaleString memorial_dialog[] = {
    LocaleString::memorial_str,
    LocaleString::memorial_str_commentary,
    LocaleString::empty};


static const LocaleString knocked_out_peer_dialog[] = {
    LocaleString::revive_peer,
    LocaleString::empty};


const LocaleString* Signpost::get_dialog() const
{
    switch (type_) {
    case Type::memorial:
        return memorial_dialog;

    case Type::lander:
        return lander_dialog;

    case Type::knocked_out_peer:
        return knocked_out_peer_dialog;
    }

    return memorial_dialog;
}


DeferredState Signpost::resume_state(Platform& pfrm, Game& game) const
{
    switch (type_) {
    case Type::memorial:
    case Type::lander:
        break;

    case Type::knocked_out_peer:
        return make_deferred_state<MultiplayerReviveState>();
    }

    return make_deferred_state<ActiveState>();
}
