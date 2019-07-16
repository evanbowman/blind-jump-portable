#pragma once

#include "buffer.hpp"
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


template <typename T>
WallCollisions check_wall_collisions(TileMap& tiles, T& entity)
{
    using Wall = Vec2<s32>;
    Buffer<Wall, 16> adjacency_vector;

    const Vec2<s32> pos = entity.get_position().template cast<s32>();
    const Vec2<TIdx> tile_coords = to_tile_coord(pos);

    auto tile_is_wall = [&](Tile t) {
        return t == Tile::none or t == Tile::ledge or t == Tile::grass_ledge;
    };

    auto check_wall = [&](TIdx x, TIdx y) {
        if (tile_is_wall(tiles.get_tile(x, y))) {
            adjacency_vector.push_back(to_world_coord<s32>({x, y}));
        }
    };

    // A four by four block around the entity should be a large enough region to
    // check for wall collisions.
    for (TIdx x = tile_coords.x - 2; x < tile_coords.x + 3; ++x) {
        for (TIdx y = tile_coords.y - 2; y < tile_coords.y + 3; ++y) {
            check_wall(x, y);
        }
    }

    WallCollisions result;

    for (const auto& wall : adjacency_vector) {
        // FIXME: this edge collision code is junk left over from the original
        // codebase.
        if ((pos.x + 6 < (wall.x + 32) and (pos.x + 6 > (wall.x))) and
            (abs((pos.y + 16) - wall.y) <= 13)) {
            result.left = true;
        }
        if ((pos.x + 26 > (wall.x) and (pos.x + 26 < (wall.x + 32))) and
            (abs((pos.y + 16) - wall.y) <= 13)) {
            result.right = true;
        }
        if (((pos.y + 22 < (wall.y + 26)) and (pos.y + 22 > (wall.y))) and
            (abs((pos.x) - wall.x) <= 16)) {
            result.up = true;
        }
        if (((pos.y + 36 > wall.y) and (pos.y + 36 < wall.y + 26)) and
            (abs((pos.x) - wall.x) <= 16)) {
            result.down = true;
        }
    }

    return result;
}
