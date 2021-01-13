#pragma once

#include "bitvector.hpp"
#include "color.hpp"
#include "number/numeric.hpp"


using TextureIndex = u16;


class Sprite {
public:
    enum Alpha : u8 {
        opaque,
        translucent,
        transparent, // invisible
        count
    };

    enum Size : u8 { w32_h32, w16_h32 };

    enum Flags1 : u8 {};
    enum Flags2 : u8 {};


    Sprite();


    using Rotation = s16;
    using Scale = Vec2<s16>;


    void set_rotation(Rotation rot);


    void set_scale(const Scale& scale);


    void set_position(const Vec2<Float>& position);


    void set_origin(const Vec2<s16>& origin);


    void set_texture_index(TextureIndex texture_index);


    void set_flip(const Vec2<bool>& flip);


    void set_alpha(Alpha alpha);


    void set_mix(const ColorMix& mix);


    void set_size(Size size);


    const Vec2<Float>& get_position() const;


    const Vec2<s16>& get_origin() const;


    TextureIndex get_texture_index() const;


    Vec2<bool> get_flip() const;


    Alpha get_alpha() const;


    const ColorMix& get_mix() const;


    Size get_size() const;


    Rotation get_rotation() const;


    Scale get_scale() const;


private:
    // For the gameboy advance edition of the game, all the data for the engine
    // is designed to fit within IWRAM, so we need to be careful about
    // memory. Packing the engine into 32kB has benefits for other platforms
    // too--this game is very cache-friendly.
    u8 alpha_ : 2;
    u8 size_ : 1;
    bool flip_x_ : 1;
    bool flip_y_ : 1;

    // Extra flags reserved for future use.
    u8 flags1_ : 3;
    u8 flags2_ : 8;

    // Because sprites are only 16x32 or 32x32, 16bits for the origin field is
    // quite generous...
    Vec2<s16> origin_;

    Vec2<s16> scale_;
    Vec2<Float> position_;
    Rotation rot_ = 0;
    TextureIndex texture_index_ = 0;
    ColorMix mix_;
};


// NOTE: If you see two texture indices with the same value, don't
// panic! The program supports both 32px and 16px wide sprites, so two
// of the same indices might refer to different sprites due to
// different sprite sizes.
enum TextureMap : TextureIndex {
    player_still_down = 63,
    player_walk_down = 64,
    player_walk_up = 69,
    player_still_up = 74,
    player_walk_left = 75,
    player_still_left = 81,
    player_walk_right = 82,
    player_still_right = 88,
    player_dodge_left = 98,
    player_dodge_right = 99,
    player_dodge_down = 100,
    player_dodge_up = 101,
    player_sidestep_right = 102,
    player_sidestep_left = 103,
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
    h_laser = 71,
    v_laser = 72,
    h_blaster = 73,
    v_blaster = 74,
    explosion = 75,
    rubble = 81,
    drone = 82,
    scarecrow_top = 83,
    scarecrow_bottom = 84,
    scarecrow_top_2 = 85,
    laser_pop = 86,
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

    inline void reverse(Sprite& sprite, Microseconds dt)
    {
        const auto& cmix = sprite.get_mix();
        if (cmix.color_ not_eq ColorConstant::null and cmix.amount_ < 255) {
            timer_ += dt;
            if (timer_ > Interval) {
                timer_ -= Interval;
                sprite.set_mix({cmix.color_, u8(cmix.amount_ + 5)});
            }
        } else {
            timer_ = 0;
            sprite.set_mix({});
        }
    }

private:
    Microseconds timer_ = 0;
};
