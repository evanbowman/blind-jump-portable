#pragma once

#include "number/numeric.hpp"
#include "pool.hpp"
#include <optional>


//
// A class for Reference Counted objects.
//
// This program targets embedded systems, and intentionally doesn't
// link with the C++ standard library (although we use some standard
// headers, like <type_traits>). Otherwise I might use
// std::shared_ptr... Rc differs from the standard smart pointers in a
// few other ways though:
// 1) No upcasting.
// 2) Can not contain null.
//
// Additionally, you need to supply a memory pool when creating an Rc<>.
//


template <typename T, u32 Count> class RcBase {
public:
    size_t strong_count() const
    {
        return control_->strong_count_;
    }

    struct ControlBlock {
        template <typename... Args>
        ControlBlock(ObjectPool<ControlBlock, Count>* pool,
                     void (*finalizer_hook)(ControlBlock*),
                     Args&&... args)
            : data_(std::forward<Args>(args)...), pool_(pool),
              finalizer_hook_(finalizer_hook), strong_count_(0), weak_count_(0)
        {
            if (finalizer_hook_ == nullptr) {
                finalizer_hook_ = [](ControlBlock* ctrl) {
                    ctrl->pool_->post(ctrl);
                };
            }
        }

        T data_;
        ObjectPool<ControlBlock, Count>* pool_;
        // Because the pool is an input parameter, I do not see much reason to
        // allow custom finalizers, but in any event, having the option to
        // customize deallocation might be useful in some unforseen way.
        void (*finalizer_hook_)(ControlBlock*);
        Atomic<u32> strong_count_;
        Atomic<u32> weak_count_;
    };

protected:
    ControlBlock* control_;

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
            control_->finalizer_hook_(control_);
        }
    }

    void remove_weak()
    {
        if (--control_->weak_count_ == 0 and control_->strong_count_ == 0) {
            control_->finalizer_hook_(control_);
        }
    }
};


template <typename T, u32 Count> class Rc : public RcBase<T, Count> {
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

    T* get() const
    {
        return &Super::control_->data_;
    }

    ~Rc()
    {
        Super::remove_strong();
    }

    template <typename... Args>
    static std::optional<Rc>
    create(ObjectPool<typename RcBase<T, Count>::ControlBlock, Count>* pool,
           void (*finalizer_hook)(typename RcBase<T, Count>::ControlBlock*),
           Args&&... args)
    {
        auto ctrl =
            pool->get(pool, finalizer_hook, std::forward<Args>(args)...);
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

    template <typename, u32> friend class Weak;
};


template <typename T, u32 Count> class Weak : public RcBase<T, Count> {
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
