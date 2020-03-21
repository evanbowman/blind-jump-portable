#pragma once

#include "color.hpp"
#include "number/numeric.hpp"


using TextureIndex = u32;


class Sprite {
public:
    enum class Alpha : u8 {
        opaque,
        translucent,
        transparent // invisible
    };

    enum class Size : u8 { w32_h32, w16_h32 };

    using Rotation = u16;


    Sprite(Size size = Size::w32_h32);


    void set_position(const Vec2<Float>& position);


    void set_origin(const Vec2<s32>& origin);


    void set_texture_index(TextureIndex texture_index);


    void set_flip(const Vec2<bool>& flip);


    void set_alpha(Alpha alpha);


    void set_mix(const ColorMix& mix);


    void set_size(Size size);


    void set_rotation(Rotation degrees);


    const Vec2<Float>& get_position() const;


    const Vec2<s32>& get_origin() const;


    TextureIndex get_texture_index() const;


    const Vec2<bool>& get_flip() const;


    Alpha get_alpha() const;


    const ColorMix& get_mix() const;


    Size get_size() const;


    Rotation get_rotation() const;


private:
    Vec2<Float> position_;
    Vec2<s32> origin_;
    Vec2<bool> flip_;
    Rotation rotation_;
    TextureIndex texture_index_;
    Alpha alpha_;
    Size size_;
    ColorMix mix_;
};


// NOTE: If you see two texture indices with the same value, don't
// panic! The program supports both 32px and 16px wide sprites, so two
// of the same indices might refer to different sprites due to
// different sprite sizes.
enum TextureMap : TextureIndex {
    player_still_down = 0,
    player_walk_down = 1,
    player_walk_up = 6,
    player_still_up = 11,
    player_walk_left = 16,
    player_still_left = 22,
    player_walk_right = 23,
    player_still_right = 29,
    transporter = 15,
    item_chest = 12,
    turret = 18,
    turret_shadow = 23,
    dasher_idle = 24,
    dasher_crouch = dasher_idle + 1,
    dasher_dash = dasher_crouch + 1,
    dasher_weapon1 = dasher_dash + 1,
    dasher_weapon2 = dasher_weapon1 + 1,
    dasher_head = dasher_weapon2 + 1,
    drop_shadow = 60,
    heart = 61,
    coin = 62,
    orb1 = 63,
    orb2 = 64,
    snake_body = 65,
    snake_head_profile = 66,
    snake_head_down = 67,
    snake_head_up = 68,
    h_laser = 69,
    h_laser2 = 70,
    v_laser = 71,
    v_laser2 = 72,
    h_blaster = 73,
    v_blaster = 74,
    explosion = 75,
    rubble = 81,
    drone = 82,
};


// Warning: The class requires the initial mix amount passed in to be a multiple
// of five. There is a reason for this, and it has to do with smooth animations
// when the game logic is run synchronously (dt is > 1000 microseconds per
// step).
template <Microseconds Interval> class FadeColorAnimation {
public:
    inline void advance(Sprite& sprite, Microseconds dt)
    {
        const auto& cmix = sprite.get_mix();
        if (cmix.amount_ > 0) {
            timer_ += dt;
            if (timer_ > Interval) {
                timer_ -= Interval;
                sprite.set_mix({cmix.color_, u8(cmix.amount_ - 5)});
            }
        } else {
            timer_ = 0;
            sprite.set_mix({});
        }
    }

private:
    Microseconds timer_ = 0;
};
