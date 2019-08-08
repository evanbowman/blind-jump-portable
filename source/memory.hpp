#pragma once

#include "numeric.hpp"
#include <array>
#include <new>
#include <optional>


template <u32 size, u32 count, u32 align = size> class Pool {
private:
    struct Cell {
        alignas(align) std::array<byte, size> mem_;
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
        pool_.post((byte*)obj);
    }

private:
    Pool<sizeof(T), count, alignof(T)> pool_;
};


// This program targets embedded systems, and intentionally doesn't
// link with the standard library (although we use some standard
// headers, like <type_traits>). Otherwise I might use
// std::shared_ptr... Rc differs from the standard smart pointers in a
// few other ways though:
// 1) No upcasting.
// 2) Can not contain null.

template <typename T, u32 Count>
class RcBase {
public:
    size_t strong_count() const
    {
        return control_->strong_count_;
    }

protected:
    struct ControlBlock {
        template <typename ...Args>
        ControlBlock(Args&& ...args) : data_(std::forward<Args>(args)...),
                                       strong_count_(0),
                                       weak_count_(0)
        {
        }

        T data_;
        Atomic<u32> strong_count_;
        Atomic<u32> weak_count_;
    };

    ControlBlock* control_;

    static auto& pool()
    {
        static ObjectPool<ControlBlock, Count> p;
        return p;
    }

    void add_strong(ControlBlock* source)
    {
        control_ = source;
        control_->strong_count_++;
    }

    void add_weak(ControlBlock* source)
    {
        control_ = source;
        control_->weak_count_++;
    }

    void remove_strong()
    {
        if (--control_->strong_count_ == 0 and control_->weak_count_ == 0) {
            pool().post(control_);
        }
    }

    void remove_weak()
    {
        if (--control_->weak_count_ == 0 and control_->strong_count_ == 0) {
            pool().post(control_);
        }
    }
};


template <typename T, u32 Count>
class Rc : public RcBase<T, Count> {
public:
    using Super = RcBase<T, Count>;

    Rc(const Rc& other)
    {
        Super::add_strong(other.control_);
    }

    Rc& operator=(const Rc& other)
    {
        if (Super::control_) {
            Super::remove_strong();
        }
        Super::add_strong(other.control_);
        return *this;
    }

    T& operator*() const
    {
        return Super::control_->data_;
    }

    T* operator->() const
    {
        return &Super::control_->data_;
    }

    ~Rc()
    {
        Super::remove_strong();
    }

    template <typename ...Args>
    static std::optional<Rc> create(Args&& ...args)
    {
        auto ctrl = Super::pool().get(std::forward<Args>(args)...);
        if (ctrl) {
            return Rc(ctrl);
        } else {
            return {};
        }
    }

private:
    Rc(typename Super::ControlBlock* control)
    {
        Super::add_strong(control);
    }

    template <typename, u32>
    friend class Weak;
};


template <typename T, u32 Count>
class Weak : public RcBase<T, Count> {
public:
    using Super = RcBase<T, Count>;

    Weak() = delete;

    Weak(const Rc<T, Count>& other)
    {
        Super::add_weak(other.control_);
    }

    std::optional<Rc<T, Count>> upgrade()
    {
        if (Super::control_->strong_count_) {
            return Rc<T, Count>(Super::control_);
        } else {
            return {};
        }
    }

    ~Weak()
    {
        Super::remove_weak();
    }
};
