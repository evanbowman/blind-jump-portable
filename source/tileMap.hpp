#pragma once

#include "numeric.hpp"


enum class Tile {
    none,
    plate,
    sand,
    ledge,
    damaged_plate,
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
    void for_each(F&& proc)
    {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                proc(data_[i][j], i, j);
            }
        }
    }

    void set_tile(u16 x, u16 y, Tile tile)
    {
        data_[x % width][y % height] = tile;
    }

    Tile get_tile(u16 x, u16 y) const
    {
        return data_[x % width][y % height];
    }

private:
    Tile data_[width][height];
};
