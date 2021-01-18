#pragma once


#include "blind_jump/entity/entity.hpp"
#include "memory/buffer.hpp"

// This is just a dummy object used in some parts of the UI. Mimics the
// appearance of any entity. Sometimes, it's much easier to simply copy the
// sprites out of an entity and edit the copy, rather than needing to extend the
// interface of various existing classes to allow certain UI states to edit the
// object.

class Proxy : public Entity {
public:
    using SpriteBuffer = Buffer<Sprite, 3>;

    Proxy(const SpriteBuffer& buffer)
    {
        for (auto& spr : buffer) {
            buffer_.push_back(spr);
        }
    }

    void set_position(const Vec2<Float>& position)
    {
        position_ = position;

        for (auto& spr : buffer_) {
            spr.set_position(position);
        }
    }

    template <typename Other> Proxy(const Other& other)
    {
        if constexpr (Other::multiface_sprite) {
            for (const auto& spr : other.get_sprites()) {
                buffer_.push_back(*spr);
            }
        } else {
            buffer_.push_back(other.get_sprite());
        }
    }

    static constexpr bool multiface_sprite = true;

    auto get_sprites() const
    {
        Buffer<Sprite*, SpriteBuffer::capacity()> result;
        for (auto& spr : buffer_) {
            result.push_back(&spr);
        }
        return result;
    }

    const Sprite& get_sprite() const
    {
        return buffer_[0];
    }

    SpriteBuffer& buffer()
    {
        return buffer_;
    }

    void pulse(ColorConstant c)
    {
        for (auto& spr : buffer_) {
            const auto cmix = spr.get_mix();
            spr.set_mix({c, cmix.amount_});
        }
        mode_ = Mode::pulse;
    }

    void colorize(const ColorMix& mix)
    {
        mode_ = Mode::none;

        for (auto& spr : buffer_) {
            spr.set_mix(mix);
        }
    }

    Proxy(const Proxy& other)
    {
        for (auto spr : other.get_sprites()) {
            buffer_.push_back(*spr);
        }
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        switch (mode_) {
        case Mode::none:
            break;

        case Mode::pulse: {
            timer_ += dt;
            auto cmix = buffer_[0].get_mix();
            cmix.amount_ = u8(128 + (sine(timer_ >> 6) >> 8));
            buffer_[0].set_mix(cmix);
            break;
        }
        }
        // fade_color_anim_.advance(buffer_[0], dt);

        const auto cmix = buffer_[0].get_mix();
        for (u32 i = 1; i < buffer_.size(); ++i) {
            buffer_[i].set_mix(cmix);
        }
    }

private:
    // FIXME: Not currently using the sprite inherited from Entity, wasteful!
    SpriteBuffer buffer_;
    Microseconds timer_ = 0;
    enum class Mode { none, pulse } mode_ = Mode::none;
    // FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
