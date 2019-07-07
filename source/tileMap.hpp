#pragma once

#include "numeric.hpp"
#include <array>


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
    static constexpr u16 tile_count{width * height};

    TileMap()
    {
        TileMap::for_each([](Tile& tile, int, int) { tile = Tile::none; });
    }

    template <typename F> TileMap(F&& init)
    {
        for_each(init);
    }

    template <typename F> void for_each(F&& proc)
    {
        for (s8 i = 0; i < width; ++i) {
            for (s8 j = 0; j < height; ++j) {
                proc(data_[TileMap::index(i, j)], i, j);
            }
        }
    }

    void set_tile(s32 x, s32 y, Tile tile)
    {
        if (x < 0 or y < 0 or x > width - 1 or y > height - 1) {
            return;
        }
        data_[TileMap::index(x, y)] = tile;
    }

    Tile get_tile(s32 x, s32 y) const
    {
        if (x < 0 or y < 0 or x > width - 1 or y > height - 1) {
            return Tile::none;
        }
        return data_[TileMap::index(x, y)];
    }

private:
    u16 index(u16 x, u16 y) const
    {
        return x + y * width;
    }

    std::array<Tile, tile_count> data_;
};
