#pragma once

#include "number/numeric.hpp"
#include <array>


enum class Tile : u8 {
    none,
    plate,
    sand,
    ledge,
    damaged_plate,
    __reserved_0, // Used for rendering the parallax scrolling starfield
    sand_sprouted,
    __reserved_2,
    grass_ledge,
    grass_ledge_vines,
    t0_count,
    // Tile layer 1 is reserved for adding detail to maps, t1 renders overtop of
    // t0 (the enumerations above)
    grass_start = 1,
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

    void set_tile(s32 x, s32 y, Tile tile);

    Tile get_tile(s32 x, s32 y) const;

private:
    u16 index(u16 x, u16 y) const;

    std::array<Tile, tile_count> data_;
};


using TIdx = TileMap::Index;


template <typename Format = Float>
inline Vec2<Format> to_world_coord(const Vec2<TIdx>& tc)
{
    return Vec2<s32>{tc.x * 32, tc.y * 24}.template cast<Format>();
}


inline Vec2<TIdx> to_tile_coord(const Vec2<s32>& wc)
{
    return Vec2<s32>{
        wc.x / 32,
        fast_divide(wc.y, 24) // This division by 24 is costly, oh well...
    }
        .cast<TIdx>();
}

inline Vec2<TIdx> to_quarter_tile_coord(const Vec2<s32>& wc)
{
    return Vec2<s32>{
        wc.x / 16,
        fast_divide(wc.y, 12) // This division by 24 is costly, oh well...
    }
        .cast<TIdx>();
}

inline bool is_walkable(Tile t)
{
    return t not_eq Tile::none and t not_eq Tile::ledge and
           t not_eq Tile::grass_ledge and t not_eq Tile::grass_ledge_vines;
}

inline bool is_border(Tile t)
{
    return t == Tile::plate or t == Tile::damaged_plate;
}
