#pragma once

#include "number/numeric.hpp"
#include <array>


template <u32 bits> class Bitvector {
public:
    Bitvector(u8 init)
    {
        static_assert(sizeof(init) * 8 == bits);
        u8* data = &init;
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] = data[i];
        }
    }

    Bitvector()
    {
        clear();
    }

    void set(u32 index, bool value)
    {
        auto& byte = data_[index / 8];
        const auto bit = index % 8;

        if (value) {
            byte = byte | (1 << bit);
        } else {
            byte &= ~(1 << bit);
        }
    }

    bool operator[](u32 index) const
    {
        return get(index);
    }

    bool get(u32 index) const
    {
        auto& byte = data_[index / 8];
        const auto bit = index % 8;
        const u8 mask = (1 << bit);
        return byte & mask;
    }

    void clear()
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
    bool get(int x, int y) const
    {
        return data_.get(y * width + x);
    }

    void set(int x, int y, bool val)
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

private:
    Bitvector<width * height> data_;
};
