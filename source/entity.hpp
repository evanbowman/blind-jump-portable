#pragma once

#include "pool.hpp"


template <typename Impl, u32 SpawnLimit>
class Entity {
public:
    Entity() : kill_flag_(false)
    {

    }

    void kill()
    {
        kill_flag_ = true;
    }

    bool alive() const
    {
        return kill_flag_;
    }

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
    template<typename F = void>
    static auto& pool()
    {
        static ObjectPool<Impl, SpawnLimit> obj_pool;
        return obj_pool;
    }

private:
    bool kill_flag_;
};
