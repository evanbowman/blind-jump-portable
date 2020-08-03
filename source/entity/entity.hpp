#pragma once

#include <algorithm>

#include "graphics/sprite.hpp"


class Platform;
class Game;

class Entity {
public:
    using Health = s32;
    using Id = u32;


    Entity();


    Entity(Health health);


    Entity(Entity&) = delete;


    void update(Platform&, Game&, Microseconds);


    Health get_health() const;


    bool alive() const;


    const Sprite& get_sprite() const;
    Sprite& get_sprite();


    void add_health(Health amount);


    const Vec2<Float>& get_position() const;


    static constexpr bool multiface_sprite = false;
    static constexpr bool has_shadow = false;
    static constexpr bool multiface_shadow = false;


    void set_health(Health health)
    {
        health_ = health;
    }


    bool visible() const
    {
        return visible_;
    }


    static void reset_ids();


    Id id() const
    {
        return id_;
    }


    // This is VERY BAD CODE. Basically, the rendering loop already determines
    // which objects are visible within the window when drawing sprites. To save
    // CPU cycles, we are marking an object visible during a rendering pass, so
    // that the game logic skip updates for certain Entities outside the window.
    void mark_visible(bool visible)
    {
        visible_ = visible;
    }

    void on_death(Platform&, Game&)
    {
    }

protected:
    void debit_health(Health amount = 1)
    {
        health_ = std::max(Health(0), health_ - amount);
    }

    void set_position(const Vec2<Float>& position);

    void kill()
    {
        health_ = 0;
    }

    Sprite sprite_;
    Vec2<Float> position_;

private:
    Health health_;
    Id id_;
    bool visible_ = false;
};


#include <memory>


template <typename T> using EntityRef = std::unique_ptr<T, void (*)(T*)>;
