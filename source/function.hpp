#pragma once

#include <array>
#include <ciso646>
#include <stdint.h>


// A fixed-space version of std::function, does not allocate.


template <std::size_t storage, typename T> class Function {
};


template <std::size_t storage, typename R, typename... Args>
class Function<storage, R(Args...)> {
public:
    Function()
        : invoke_policy_(nullptr), construct_policy_(nullptr),
          move_policy_(nullptr), destroy_policy_(nullptr), data_(nullptr),
          data_size_(0)
    {
    }


    template <typename Functor>
    Function(Functor f)
        : invoke_policy_(reinterpret_cast<InvokePolicy>(invokeImpl<Functor>)),
          construct_policy_(
              reinterpret_cast<ConstructPolicy>(constructImpl<Functor>)),
          move_policy_(reinterpret_cast<MovePolicy>(moveImpl<Functor>)),
          destroy_policy_(
              reinterpret_cast<DestroyPolicy>(destroyImpl<Functor>)),
          data_(nullptr), data_size_(sizeof(Functor))
    {
        static_assert(storage >= sizeof(Functor));

        if (sizeof(Functor) <= internal_storage_.size()) {
            data_ = internal_storage_.data();
        }
        static_assert(alignof(Functor) <= alignof(decltype(data_)),
                      "Function uses a hard-coded maximum alignment of"
                      " eight bytes. You can increase it to 16 or higher if"
                      " it\'s really necessary...");
        construct_policy_(data_, reinterpret_cast<void*>(&f));
    }


    Function(Function const& rhs)
        : invoke_policy_(rhs.invoke_policy_),
          construct_policy_(rhs.construct_policy_),
          move_policy_(rhs.move_policy_), destroy_policy_(rhs.destroy_policy_),
          data_(nullptr), data_size_(rhs.data_size_)
    {
        if (invoke_policy_) {
            data_ = internal_storage_.data();
            construct_policy_(data_, rhs.data_);
        }
    }


    Function(Function&& rhs)
        : invoke_policy_(rhs.invoke_policy_),
          construct_policy_(rhs.construct_policy_),
          move_policy_(rhs.move_policy_), destroy_policy_(rhs.destroy_policy_),
          data_(nullptr), data_size_(rhs.data_size_)
    {
        rhs.invoke_policy_ = nullptr;
        rhs.construct_policy_ = nullptr;
        rhs.destroy_policy_ = nullptr;
        if (invoke_policy_) {
            data_ = internal_storage_.data();
            move_policy_(data_, rhs.data_);
            rhs.data_ = nullptr;
        }
    }


    ~Function()
    {
        if (data_ not_eq nullptr) {
            destroy_policy_(data_);
        }
    }


    R operator()(Args&&... args)
    {
        return invoke_policy_(data_, std::forward<Args>(args)...);
    }


private:
    typedef R (*InvokePolicy)(void*, Args&&...);
    typedef void (*ConstructPolicy)(void*, void*);
    typedef void (*MovePolicy)(void*, void*);
    typedef void (*DestroyPolicy)(void*);


    template <typename Functor> static R invokeImpl(Functor* fn, Args&&... args)
    {
        return (*fn)(std::forward<Args>(args)...);
    }


    template <typename Functor>
    static void constructImpl(Functor* construct_dst, Functor* construct_src)
    {
        new (construct_dst) Functor(*construct_src);
    }


    template <typename Functor>
    static void moveImpl(Functor* move_dst, Functor* move_src)
    {
        new (move_dst) Functor(std::move(*move_src));
    }


    template <typename Functor> static void destroyImpl(Functor* f)
    {
        f->~Functor();
    }


    InvokePolicy invoke_policy_;
    ConstructPolicy construct_policy_;
    MovePolicy move_policy_;
    DestroyPolicy destroy_policy_;
    // TODO: 16 Or higher alignment is a somewhat unusual edge case, but the
    // code _should_ be updated to handle it.
    alignas(8) std::array<uint8_t, storage> internal_storage_;

    // FIXME: data_ is leftover member variable that isn't really needed anymore
    void* data_;
    std::size_t data_size_;
};
