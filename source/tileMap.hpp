#pragma once

#include "numeric.hpp"


enum class Tile {
    none,
    plate,
    sand,
};


class TileMap {
public:
    static constexpr u16 width = 16;
    static constexpr u16 height = 20;

    TileMap()
    {
        fill([](int, int) { return Tile::sand; });
    }

    template <typename F>
    void fill(F&& proc)
    {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                data_[i][j] = proc(i, j);
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
