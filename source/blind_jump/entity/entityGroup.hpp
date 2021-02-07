#pragma once

#include "blind_jump/entity/entity.hpp"
#include "list.hpp"
#include "memory/pool.hpp"
#include "transformGroup.hpp"


using EntityNode = BiNode<EntityRef<Entity>>;


template <u32 Capacity>
using EntityNodePool = Pool<sizeof(EntityNode), Capacity, alignof(EntityNode)>;


template <typename Arg, u32 Capacity>
using EntityBuffer = List<EntityRef<Arg>, EntityNodePool<Capacity>>;


template <size_t Capacity, typename... Members>
class EntityGroup : public TransformGroup<EntityBuffer<Members, Capacity>...> {
public:
    using Pool_ = Pool<std::max({sizeof(Members)...}),
                       Capacity,
                       std::max({alignof(Members)...})>;

    using NodePool_ = EntityNodePool<Capacity>;

    EntityGroup(Pool_& pool, NodePool_& node_pool)
        : TransformGroup<EntityBuffer<Members, Capacity>...>(node_pool)
    {
        node_pool_ = &node_pool;
        pool_ = &pool;
    }

    template <typename T, typename... CtorArgs> T* spawn(CtorArgs&&... ctorArgs)
    {
        auto deleter = [](T* obj) {
            if (obj) {
                obj->~T();
                pool_->post(reinterpret_cast<byte*>(obj));
            }
        };

        if (auto mem = pool_->get()) {
            new (mem) T(std::forward<CtorArgs>(ctorArgs)...);

            this->get<T>().push({reinterpret_cast<T*>(mem), deleter});

            return reinterpret_cast<T*>(mem);
        } else {
            return nullptr;
        }
    }

    template <typename T> auto& get()
    {
        return TransformGroup<EntityBuffer<Members, Capacity>...>::template get<
            EntityBuffer<T, Capacity>>();
    }

    template <int n> auto& get()
    {
        return TransformGroup<EntityBuffer<Members, Capacity>...>::template get<
            n>();
    }

    template <typename T> static constexpr int index_of()
    {
        return TransformGroup<EntityBuffer<Members, Capacity>...>::
            template index_of<EntityBuffer<T, Capacity>>();
    }

    void clear()
    {
        this->template transform([](auto& buf) { buf.clear(); });
    }

private:
    static NodePool_* node_pool_;

    static Pool_* pool_;
};


template <size_t Cap, typename... Members>
typename EntityGroup<Cap, Members...>::Pool_*
    EntityGroup<Cap, Members...>::pool_;

template <size_t Cap, typename... Members>
typename EntityGroup<Cap, Members...>::NodePool_*
    EntityGroup<Cap, Members...>::node_pool_;
