#include "game.hpp"
#include <algorithm>


void Game::update(Platform& pfrm, Microseconds delta)
{
    player_.update(pfrm, *this, delta);

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            (*it)->update(pfrm, *this, delta);
        }
    };

    enemies_.transform(update_policy);
    details_.transform(update_policy);
    effects_.transform(update_policy);

    camera_.update(pfrm, delta, player_.get_position());

    display_buffer.push_back(&player_.get_sprite());

    auto render_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            display_buffer.push_back(&(*it)->get_sprite());
        }
    };

    enemies_.transform(render_policy);
    details_.transform(render_policy);
    effects_.transform(render_policy);

    display_buffer.push_back(&transporter_.get_sprite());

    if (pfrm.keyboard().down_transition<Keyboard::Key::action_1>()) {
        Game::next_level(pfrm);
    }

    std::sort(display_buffer.begin(),
              display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });

    display_buffer.push_back(&player_.get_shadow());
}


void Game::render(Platform& pfrm)
{
    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }

    display_buffer.clear();
}


static void condense(TileMap& map, TileMap& maptemp)
{
    map.for_each([&](const Tile& tile, int x, int y) {
        uint8_t count = 0;
        auto collect = [&](int x, int y) {
            if (map.get_tile(x, y) == Tile::none) {
                count++;
            }
        };
        collect(x - 1, y - 1);
        collect(x + 1, y - 1);
        collect(x - 1, y + 1);
        collect(x + 1, y + 1);
        collect(x - 1, y);
        collect(x + 1, y);
        collect(x, y - 1);
        collect(x, y + 1);
        if (tile == Tile::none) {
            if (count < 4) {
                maptemp.set_tile(x, y, Tile::plate);
            } else {
                maptemp.set_tile(x, y, Tile::none);
            }
        } else {
            if (count > 5) {
                maptemp.set_tile(x, y, Tile::none);
            } else {
                maptemp.set_tile(x, y, Tile::plate);
            }
        }
    });
    maptemp.for_each(
        [&](const Tile& tile, int x, int y) { map.set_tile(x, y, tile); });
}


Game::Game(Platform& pfrm) : level_(-1)
{
    Game::next_level(pfrm);
}


void Game::next_level(Platform& pfrm)
{
    level_ += 1;
    Game::regenerate_map(pfrm);
    Game::respawn_entities(pfrm);
    pfrm.push_map(tiles_);
}


void Game::regenerate_map(Platform& platform)
{
    TileMap temporary;

    tiles_.for_each([&](Tile& t, int, int) {
        t = Tile(random_choice<int(Tile::sand)>(platform));
    });

    for (int i = 0; i < 3; ++i) {
        condense(tiles_, temporary);
    }

    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (x == 0 or x == TileMap::width - 1 or y == 0 or
            y == TileMap::height - 1) {
            tile = Tile::none;
        }
    });

    TileMap grass_overlay([&](Tile& t, int, int) {
        if (random_choice<3>(platform)) {
            t = Tile::none;
        } else {
            t = Tile::plate;
        }
    });

    for (int i = 0; i < 2; ++i) {
        condense(grass_overlay, temporary);
    }

    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::plate and
            tiles_.get_tile(x - 1, y) not_eq Tile::none and
            tiles_.get_tile(x + 1, y) not_eq Tile::none and
            tiles_.get_tile(x, y - 1) not_eq Tile::none and
            tiles_.get_tile(x, y + 1) not_eq Tile::none) {
            tile = Tile::sand;
        }
    });

    tiles_.for_each([&](Tile& tile, int x, int y) {
        auto above = tiles_.get_tile(x, y - 1);
        if (tile == Tile::none and
            (above == Tile::plate or above == Tile::damaged_plate)) {
            tile = Tile::ledge;
        }
    });

    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::none) {
            grass_overlay.set_tile(x, y, Tile::none);
        }
    });

    u8 bitmask[TileMap::width][TileMap::height];
    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            bitmask[x][y] = 0;
            bitmask[x][y] +=
                1 * static_cast<int>(grass_overlay.get_tile(x, y - 1));
            bitmask[x][y] +=
                2 * static_cast<int>(grass_overlay.get_tile(x + 1, y));
            bitmask[x][y] +=
                4 * static_cast<int>(grass_overlay.get_tile(x, y + 1));
            bitmask[x][y] +=
                8 * static_cast<int>(grass_overlay.get_tile(x - 1, y));
        }
    }

    grass_overlay.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::plate) {
            auto match = tiles_.get_tile(x, y);
            u8 val;
            switch (match) {
            case Tile::plate:
                val = (int)Tile::grass_plate + bitmask[x][y];
                tiles_.set_tile(x, y, (Tile)val);
                break;

            case Tile::sand:
                val = (int)Tile::grass_sand + bitmask[x][y];
                tiles_.set_tile(x, y, (Tile)val);
                break;

            case Tile::ledge:
                tiles_.set_tile(x, y, Tile::grass_ledge);
                break;

            default:
                break;
            }
        }
    });

    tiles_.for_each([&](Tile& tile, int, int) {
        if (tile == Tile::plate) {
            if (random_choice<8>(platform) == 0) {
                tile = Tile::damaged_plate;
            }
        }
    });
}


void Game::respawn_entities(Platform& pfrm)
{
    auto clear_entities = [&](auto& buf) { buf.clear(); };

    enemies_.transform(clear_entities);
    details_.transform(clear_entities);
    effects_.transform(clear_entities);

    using Coord = Vec2<s8>;
    Buffer<Coord, TileMap::tile_count> free_spots;

    tiles_.for_each([&](Tile& tile, s8 x, s8 y) {
        if (tile == Tile::sand or (u8(tile) >= u8(Tile::grass_sand) and
                                   u8(tile) < u8(Tile::grass_plate))) {
            free_spots.push_back({x, y});
        }
    });

    auto select_coord = [&]() -> Coord* {
        if (not free_spots.empty()) {
            auto result = &free_spots[random_choice(pfrm, free_spots.size())];
            free_spots.erase(result);
            return result;
        } else {
            return nullptr;
        }
    };

    auto pos = [&](Coord* c) {
        return Vec2<float>{static_cast<Float>(c->x * 32),
                           static_cast<Float>(c->y * 24)};
    };

    if (const auto c = select_coord()) {
        details_.get<0>().push_back(ItemChest::pool().get(pos(c)));
    }

    if (const auto c = select_coord()) {
        transporter_.set_position(pos(c));
    }
}
