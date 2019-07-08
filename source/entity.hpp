#pragma once

#include "pool.hpp"
#include "sprite.hpp"


class EntityBase {
public:
    EntityBase() : kill_flag_(false)
    {
    }

    EntityBase(EntityBase&) = delete;

    bool alive() const
    {
        return kill_flag_;
    }

    const Sprite& get_sprite() const
    {
        return sprite_;
    }

protected:
    void kill()
    {
        kill_flag_ = true;
    }

    Sprite sprite_;

private:
    bool kill_flag_;
};


template <typename Impl, u32 SpawnLimit> class Entity : public EntityBase {
public:
    static constexpr auto spawn_limit()
    {
        return SpawnLimit;
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
