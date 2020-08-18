#pragma once

#include "number/numeric.hpp"
#include <array>


template <u32 bits> class Bitvector {
public:
    explicit constexpr Bitvector(u8 init) : data_{init}
    {
        static_assert(sizeof(init) * 8 == bits);
    }

    constexpr Bitvector(const std::array<bool, bits> init) : data_({})
    {
        for (std::size_t bit = 0; bit < init.size(); ++bit) {
            this->set(bit, init[bit]);
        }
    }

    constexpr Bitvector() : data_({})
    {
    }

    Bitvector& operator=(const Bitvector& other)
    {
        data_ = other.data_;
        return *this;
    }

    constexpr u32 size() const
    {
        return bits;
    }

    constexpr void set(u32 index, bool value)
    {
        auto& byte = data_[index / 8];
        const auto bit = index % 8;

        if (value) {
            byte = byte | (1 << bit);
        } else {
            byte &= ~(1 << bit);
        }
    }

    constexpr bool get(u32 index) const
    {
        auto& byte = data_[index / 8];
        const auto bit = index % 8;
        const u8 mask = (1 << bit);
        return byte & mask;
    }

    constexpr bool operator[](u32 index) const
    {
        return get(index);
    }

    constexpr void clear()
    {
        for (u8& byte : data_) {
            byte = 0;
        }
    }

private:
    std::array<u8, (bits / 8) + ((bits % 8) ? 1 : 0)> data_;
};


template <int width, int height> class Bitmatrix {
public:
    constexpr Bitmatrix() : data_{}
    {
    }

    constexpr Bitmatrix(const std::array<std::array<bool, height>, width>& init)
    {
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                this->set(x, y, init[x][y]);
            }
        }
    }

    constexpr bool get(int x, int y) const
    {
        return data_.get(y * width + x);
    }

    constexpr void set(int x, int y, bool val)
    {
        static_assert(width % 8 == 0,
                      "Warning: this code runs faster when you use power-of-two"
                      " sizes... remove this warning if you performance is"
                      " unimportant to your use case.");
        data_.set(y * width + x, val);
    }

    void clear()
    {
        data_.clear();
    }

    constexpr Vec2<int> size() const
    {
        return {width, height};
    }

private:
    Bitvector<width * height> data_;
};
