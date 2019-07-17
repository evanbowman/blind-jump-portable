#pragma once

#include "pool.hpp"
#include "sprite.hpp"


class EntityBase {
public:
    using Health = s32;

    EntityBase() : health_(1)
    {
    }

    EntityBase(Health health) : health_(health)
    {
    }

    EntityBase(EntityBase&) = delete;

    Health get_health() const
    {
        return health_;
    }

    bool alive() const
    {
        return health_ not_eq 0;
    }

    const Sprite& get_sprite() const
    {
        return sprite_;
    }

    void set_position(const Vec2<Float>& position)
    {
        position_ = position;
    }

    const Vec2<Float>& get_position() const
    {
        return position_;
    }

protected:
    void debit_health(Health amount)
    {
        health_ = std::max(Health(0), health_ - amount);
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

    // Note: Impl is still an incomplete type at this point, so we need to delay
    // instantiation of the Pool until access. Otherwise it could just be a
    // static member.
    //
    // The game is designed to run on a wide variety of platforms, including
    // consoles without an OS, so Entities are allocated from fixed pools.
    //
    template <typename F = void> static auto& pool()
    {
        static ObjectPool<Impl, SpawnLimit> obj_pool;
        return obj_pool;
    }
};


#include <memory>


template <typename T> using EntityRef = std::unique_ptr<T, void (*)(T*)>;


template <typename T, typename... Args> EntityRef<T> make_entity(Args&&... args)
{
    return {T::pool().get(std::forward<Args>(args)...),
            [](T* mem) { T::pool().post(mem); }};
}
