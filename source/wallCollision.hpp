#pragma once

#include "memory/buffer.hpp"
#include "tileMap.hpp"


struct WallCollisions {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    bool any() const
    {
        return up or down or left or right;
    }
};


using Wall = Vec2<s32>;
using WallAdjacencyVector = Buffer<Wall, 16>;


template <typename T>
WallAdjacencyVector adjacent_walls(TileMap& tiles, const T& entity)
{
    WallAdjacencyVector v;
    Vec2<s32> pos = entity.get_position().template cast<s32>();
    pos.y += 2;

    const Vec2<TIdx> tile_coords = to_tile_coord(pos);

    auto check_wall = [&](TIdx x, TIdx y) {
        if (not is_walkable(tiles.get_tile(x, y))) {
            v.push_back(to_world_coord<s32>({x, y}));
        }
    };

    // A four by four block around the entity should be a large enough region to
    // check for wall collisions.
    for (TIdx x = tile_coords.x - 1; x < tile_coords.x + 2; ++x) {
        for (TIdx y = tile_coords.y - 1; y < tile_coords.y + 2; ++y) {
            check_wall(x, y);
        }
    }

    return v;
}


template <typename T>
WallCollisions check_wall_collisions(TileMap& tiles, T& entity)
{
    auto adjacency_vector = adjacent_walls(tiles, entity);

    Vec2<s32> pos = entity.get_position().template cast<s32>();
    pos.y += 2;

    WallCollisions result;

    for (const auto& wall : adjacency_vector) {
        // FIXME: this edge collision code is junk left over from the original
        // codebase.
        if ((pos.x - 16 + 6 < (wall.x + 32) and (pos.x - 16 + 6 > (wall.x))) and
            (abs((pos.y - 16 + 16) - wall.y) <= 13)) {
            result.left = true;
        }
        if ((pos.x - 16 + 26 > (wall.x) and
             (pos.x - 16 + 26 < (wall.x + 32))) and
            (abs((pos.y - 16 + 16) - wall.y) <= 13)) {
            result.right = true;
        }
        if (((pos.y - 16 + 22 < (wall.y + 26)) and
             (pos.y - 16 + 22 > (wall.y))) and
            (abs((pos.x - 16) - wall.x) <= 16)) {
            result.up = true;
        }
        if (((pos.y - 16 + 36 > wall.y) and (pos.y - 16 + 36 < wall.y + 26)) and
            (abs((pos.x - 16) - wall.x) <= 16)) {
            result.down = true;
        }
    }

    return result;
}
