#pragma once

#include "number/numeric.hpp"
#include <array>
#include <new>


template <u32 size, u32 count, u32 align = size> class Pool {
public:
    struct Cell {
        alignas(align) std::array<byte, size> mem_;
        Cell* next_;
    };


    Pool() : freelist_(nullptr)
    {
        for (decltype(count) i = 0; i < count; ++i) {
            Cell* next = &cells_[i];
            next->next_ = freelist_;
            freelist_ = next;
        }
    }

    Pool(const Pool&) = delete;

    byte* get()
    {
        if (freelist_) {
            const auto ret = freelist_;
            freelist_ = freelist_->next_;
            return (byte*)ret;
        } else {
            return nullptr;
        }
    }

    void post(byte* mem)
    {
        auto cell = (Cell*)mem;
        cell->next_ = freelist_;
        freelist_ = cell;
    }

    static constexpr u32 element_size()
    {
        return size;
    }

    static constexpr u32 alignment()
    {
        return align;
    }

    using Memory = std::array<Cell, count>;
    Memory& memory()
    {
        return cells_;
    }

    u32 remaining() const
    {
        const Cell* current = freelist_;
        int n = 0;
        while (current) {
            current = current->next_;
            ++n;
        }
        return n;
    }

    bool empty() const
    {
        return freelist_ == nullptr;
    }

private:
    Memory cells_;
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
        pool_.post((byte*)obj);
    }

    u32 remaining() const
    {
        return pool_.remaining();
    }

    bool empty() const
    {
        return pool_.empty();
    }

private:
    Pool<sizeof(T), count, alignof(T)> pool_;
};
