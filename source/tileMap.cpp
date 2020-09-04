#include "tileMap.hpp"


u16 TileMap::index(u16 x, u16 y) const
{
    return x + y * width;
}


void TileMap::set_tile(s32 x, s32 y, u8 tile)
{
    if (x < 0 or y < 0 or x > width - 1 or y > height - 1) {
        return;
    }
    data_[TileMap::index(x, y)] = tile;
}


u8 TileMap::get_tile(s32 x, s32 y) const
{
    if (x < 0 or y < 0 or x > width - 1 or y > height - 1) {
        return Tile::none;
    }
    return data_[TileMap::index(x, y)];
}
