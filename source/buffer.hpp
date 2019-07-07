#pragma once

#include "numeric.hpp"
#include <array>
#include <new>


template <typename T, u32 Capacity> class Buffer {
public:
    using Iterator = T*;
    using iterator = Iterator; // (only for stl compatibility)
    using ValueType = T;

    Buffer() : begin_((Iterator)mem_.data()), end_(begin_)
    {
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


    void push_back(const T& elem)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(elem);
            ++end_;
        }
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
        while (not empty())
            pop_back();
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
