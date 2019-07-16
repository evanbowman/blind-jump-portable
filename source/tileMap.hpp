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
    // NOTE: The tilemap constants here are sixteen and twenty, due to limits of
    // the GBA hardware, but could be bumped up for Desktop releases.
    static constexpr u16 width = 16;
    static constexpr u16 height = 20;
    static constexpr u16 tile_count{width * height};

    using Index = s8;

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
        for (Index i = 0; i < width; ++i) {
            for (Index j = 0; j < height; ++j) {
                proc(data_[TileMap::index(i, j)], i, j);
            }
        }
    }

    template <typename F> void for_each(F&& proc) const
    {
        for (Index i = 0; i < width; ++i) {
            for (Index j = 0; j < height; ++j) {
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


using TIdx = TileMap::Index;


template <typename Format = Float>
inline Vec2<Format> to_world_coord(const Vec2<TIdx>& tc)
{
    return Vec2<s32>{
        tc.x * 32,
        tc.y * 24
    }.template cast<Format>();
}


inline Vec2<TIdx> to_tile_coord(const Vec2<s32>& wc)
{
    return Vec2<s32>{
        wc.x / 32,
        wc.y / 24 // This division by 24 is costly, oh well...
    }.cast<TIdx>();
}
