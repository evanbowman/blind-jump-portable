#pragma once

#include <algorithm>

#include "graphics/sprite.hpp"


class Platform;
class Game;

class EntityBase {
public:
    using Health = s32;


    EntityBase();


    EntityBase(Health health);


    EntityBase(EntityBase&) = delete;


    void update(Platform&, Game&, Microseconds);


    Health get_health() const;


    bool alive() const;


    const Sprite& get_sprite() const;


    void add_health(Health amount);


    const Vec2<Float>& get_position() const;


    static constexpr bool multiface_sprite = false;


protected:
    void debit_health(Health amount)
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
};


template <typename Impl, u32 SpawnLimit> class Entity : public EntityBase {
public:
    static constexpr auto spawn_limit()
    {
        return SpawnLimit;
    }

    Entity() = default;

    Entity(Health health) : EntityBase(health)
    {
    }
};


#include <memory>


template <typename T> using EntityRef = std::unique_ptr<T, void (*)(T*)>;
