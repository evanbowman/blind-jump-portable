#pragma once

#include <array>
#include "numeric.hpp"


enum class Tile : u8 {
    none,
    plate,
    sand,
    ledge,
    damaged_plate,
    __reserved_0,
    grass_sand,
    grass_plate = grass_sand + 16,
    grass_ledge = grass_plate + 16,
    count,
};


class TileMap {
public:
    static constexpr u16 width = 16;
    static constexpr u16 height = 20;

    TileMap()
    {
        for_each([](Tile& tile, int, int) { tile = Tile::none; });
    }

    template <typename F>
    TileMap(F&& init)
    {
        for_each(init);
    }

    template <typename F>
    void for_each(F&& proc)
    {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                proc(data_[index(i, j)], i, j);
            }
        }
    }

    void set_tile(u16 x, u16 y, Tile tile)
    {
        x %= width;
        y %= height;
        data_[index(x, y)] = tile;
    }

    Tile get_tile(u16 x, u16 y) const
    {
        x %= width;
        y %= height;
        return data_[index(x, y)];
    }

private:
    u16 index(u16 x, u16 y) const
    {
        return x + y * width;
    }

    std::array<Tile, width * height> data_;
};
