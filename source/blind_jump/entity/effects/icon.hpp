#pragma once


#include "entity/entity.hpp"
#include "memory/buffer.hpp"


// This is just a dummy object used in some parts of the UI. Mimics the
// appearance of any entity. Sometimes, it's much easier to simply copy the
// sprites out of an entity and edit the copy, rather than needing to extend the
// interface of various existing classes to allow certain UI states to edit the
// object.

class Proxy {
public:
    using SpriteBuffer = Buffer<Sprite, 4>;

    template <typename Other> Proxy(const Other& other)
    {
        if constexpr (Other::multiface_sprite) {
            for (const auto& spr : other.get_sprites()) {
                buffer_.push_back(spr);
            }
        } else {
            buffer_.push_back(other.get_sprite());
        }
    }

    const SpriteBuffer& get_sprites()
    {
        return buffer_;
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        fade_color_anim_.advance(buffer_[0], dt);

        const auto cmix = buffer_[0].get_mix();
        for (u32 i = 1; i < buffer_.size(); ++i) {
            buffer_[i].set_mix(cmix);
        }
    }

private:
    SpriteBuffer buffer_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
