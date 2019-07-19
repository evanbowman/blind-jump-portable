#pragma once

#include "numeric.hpp"
#include <array>
#include <new>


template <typename T, u32 Capacity> class Buffer {
public:
    using Iterator = T*;
    using ValueType = T;

    // (only for stl compatibility)
    using iterator = Iterator;
    using value_type = ValueType;

    Buffer() : begin_((Iterator)mem_.data()), end_(begin_)
    {
    }

    ~Buffer()
    {
        Buffer::clear();
    }


    constexpr u32 capacity()
    {
        return Capacity;
    }


    Iterator begin()
    {
        return begin_;
    }


    Iterator end()
    {
        return end_;
    }

    template <typename... Args> void emplace_back(Args&&... args)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(std::forward<Args>(args)...);
            ++end_;
        }
    }

    void push_back(const T& elem)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(elem);
            ++end_;
        }
    }

    void push_back(T&& elem)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(std::forward<T>(elem));
            ++end_;
        }
    }

    T& operator[](u32 index)
    {
        return begin_[index];
    }

    // FIXME: I wrote this fn in like 30 seconds and it's not great
    void erase(Iterator slot)
    {
        slot->~T();
        for (; slot not_eq end_; ++slot) {
            // FIXME: improve this!
            if (slot + 1 not_eq end_) {
                new (slot) T(std::move(*(slot + 1)));
            }
        }
        end_ -= 1;
    }


    const T& back() const
    {
        return *(end_ - 1);
    }


    const T& front() const
    {
        return *begin_;
    }


    void pop_back()
    {
        --end_;
        end_->~T();
    }

    void clear()
    {
        while (not Buffer::empty())
            Buffer::pop_back();
    }

    u32 size() const
    {
        return end_ - begin_;
    }


    bool empty() const
    {
        return Buffer::size() == 0;
    }


private:
    alignas(T) std::array<u8, Capacity * sizeof(T)> mem_;
    Iterator begin_;
    Iterator end_;
};
