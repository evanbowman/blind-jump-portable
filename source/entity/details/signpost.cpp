#include "signpost.hpp"


Signpost::Signpost(const Vec2<Float>& position, Type type) : type_(type)
{
    position_ = position;

    switch (type_) {
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


static LocaleString memorial_dialog[] = {LocaleString::memorial_str,
                                         LocaleString::memorial_str_commentary,
                                         LocaleString::empty};


const LocaleString* Signpost::get_dialog() const
{
    switch (type_) {
    case Type::memorial:
        break;
    }

    return memorial_dialog;
}
