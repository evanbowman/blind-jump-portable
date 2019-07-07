#pragma once

#include "numeric.hpp"
#include <array>
#include <new>


template <u32 size, u32 count, u32 align = size> class Pool {
private:
    struct Cell {
        alignas(align) std::array<u8, size> mem_;
        Cell* next_;
    };

public:
    Pool() : freelist_(nullptr)
    {
        for (decltype(count) i = 0; i < count; ++i) {
            Cell* next = &cells_[i];
            next->next_ = freelist_;
            freelist_ = next;
        }
    }

    u8* get()
    {
        if (freelist_) {
            const auto ret = freelist_;
            freelist_ = freelist_->next_;
            return (u8*)ret;
        } else {
            return nullptr;
        }
    }

    void post(u8* mem)
    {
        auto cell = (Cell*)mem;
        cell->next_ = freelist_;
        freelist_ = cell;
    }

private:
    std::array<Cell, count> cells_;
    Cell* freelist_;
};


template <typename T, u32 count> class ObjectPool {
public:
    template <typename... Args> T* get(Args&&... args)
    {
        auto mem = pool_.get();
        if (mem) {
            new (mem) T(std::forward<Args>(args)...);
            return reinterpret_cast<T*>(mem);
        } else {
            return nullptr;
        }
    }

    void post(T* obj)
    {
        obj->~T();
        pool_.post((u8*)obj);
    }

private:
    Pool<sizeof(T), count, alignof(T)> pool_;
};
