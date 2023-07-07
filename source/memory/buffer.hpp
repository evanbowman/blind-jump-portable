////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2022  Evan Bowman
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of version 2 of the GNU General Public License as published by the
// Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// GPL2 ONLY. No later versions permitted.
//
////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "number/numeric.hpp"
#include "util.hpp"
#include <array>
#include <new>


template <typename T, u32 Capacity> class Buffer
{
public:
    using Iterator = T*;
    using ValueType = T;

    // (only for stl compatibility)
    using iterator = Iterator;
    using value_type = ValueType;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using ReverseIterator = reverse_iterator;


    Buffer() : mem_{}, begin_((Iterator)mem_.data()), end_(begin_)
    {
    }

    Buffer(const Buffer& other)
        : mem_{}, begin_((Iterator)mem_.data()), end_(begin_)
    {
        for (auto& elem : other) {
            push_back(elem);
        }
    }

    const Buffer& operator=(const Buffer& other)
    {
        clear();
        for (auto& elem : other) {
            push_back(elem);
        }
        return *this;
    }

    Buffer(Buffer&& other) : mem_{}, begin_((Iterator)mem_.data()), end_(begin_)
    {
        for (auto& elem : other) {
            this->push_back(std::move(elem));
        }
    }

    const Buffer& operator=(Buffer&& other)
    {
        clear();
        for (auto& elem : other) {
            this->push_back(std::move(elem));
        }
        return *this;
    }

    ~Buffer()
    {
        if constexpr (not std::is_trivially_destructible<T>()) {
            Buffer::clear();
        }
    }


    static constexpr u32 capacity()
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


    ReverseIterator rbegin()
    {
        return ReverseIterator(end());
    }


    ReverseIterator rend()
    {
        return ReverseIterator(begin());
    }


    Iterator begin() const
    {
        return begin_;
    }


    Iterator end() const
    {
        return end_;
    }


    template <typename... Args> bool emplace_back(Args&&... args)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(std::forward<Args>(args)...);
            ++end_;
            return true;
        } else {
            return false;
        }
    }

    // unsafe! Assumes that you've already checked if full or know contextually
    // that a buffer has enough space for everything that'll be pushed.
    template <typename... Args> void emplace_unsafe(Args&&... args)
    {
        // new (end_) T(std::forward<Args>(args)...);
        ++end_;
    }

    bool push_back(
        const T& elem,
        void* overflow_callback_data = nullptr,
        void (*overflow_callback)(void*) = [](void*) {})
    {
        static_assert(std::is_trivially_destructible<T>());
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(elem);
            ++end_;
            return true;
        } else {
            overflow_callback(overflow_callback_data);
            return false;
        }
    }

    // Not bounds checked!
    void push_unsafe(const T& elem)
    {
        static_assert(std::is_trivially_destructible<T>());
        new (end_) T(elem);
        ++end_;
    }

    bool push_back(T&& elem)
    {
        if (Buffer::size() < Buffer::capacity()) {
            new (end_) T(std::forward<T>(elem));
            ++end_;
            return true;
        } else {
            return false;
        }
    }

    T& operator[](u32 index)
    {
        return begin_[index];
    }

    const T& operator[](u32 index) const
    {
        return begin_[index];
    }

    Iterator insert(Iterator pos, const T& elem)
    {
        if (full()) {
            return pos;
        } else if (empty()) {
            push_back(elem);
            return begin_;
        }

        for (auto it = end_ - 1; it not_eq pos - 1; --it) {
            new (it + 1) T(*it);
            it->~T();
        }
        ++end_;

        new (pos) T(elem);
        return pos;
    }

    // FIXME: I wrote this fn in like 30 seconds and it's not great
    Iterator erase(Iterator slot)
    {
        const auto before = slot;

        slot->~T();
        for (; slot not_eq end_; ++slot) {
            // FIXME: improve this!
            if (slot + 1 not_eq end_) {
                new (slot) T(std::move(*(slot + 1)));
            }
        }
        end_ -= 1;
        return before;
    }


    T& back()
    {
        return *(end_ - 1);
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
        if constexpr (std::is_trivially_destructible<T>()) {
            end_ = begin_;
        } else {
            while (not Buffer::empty())
                Buffer::pop_back();
        }
    }

    u32 size() const
    {
        return end_ - begin_;
    }

    const T* data() const
    {
        return reinterpret_cast<const T*>(mem_.data());
    }

    bool empty() const
    {
        return begin() == end();
    }


    bool full() const
    {
        return Buffer::size() == Buffer::capacity();
    }


private:
    alignas(T) std::array<u8, Capacity * sizeof(T)> mem_;
    Iterator begin_;
    Iterator end_;
};
