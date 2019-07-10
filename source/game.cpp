#include "game.hpp"
#include <algorithm>


void Game::update(Platform& pfrm, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Keyboard::Key::action_2>()) {
        if (manhattan_length(player_.get_position(),
                             transporter_.get_position()) < 32) {
            Game::next_level(pfrm);
        }
    }

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

RETRY:
    Game::regenerate_map(pfrm);

    if (not Game::respawn_entities(pfrm)) {
        goto RETRY;
    }

    pfrm.push_map(tiles_);

    const auto player_pos = player_.get_position();
    const auto ssize = pfrm.screen().size();
    camera_.set_position(
        pfrm,
        {(player_pos.x + 16) - ssize.x / 2, player_pos.y - float(ssize.y)});
}


static u32 flood_fill(TileMap& map, Tile replace, s8 x, s8 y)
{
    using Coord = Vec2<s8>;

    Buffer<Coord, TileMap::width * TileMap::height> stack;

    const Tile target = map.get_tile(x, y);

    u32 count = 0;

    const auto action = [&](const Coord& c, s8 x_off, s8 y_off) {
        const s8 x = c.x + x_off;
        const s8 y = c.y + y_off;
        if (x > 0 and x < TileMap::width and y > 0 and y < TileMap::height) {
            if (map.get_tile(x, y) == target) {
                map.set_tile(x, y, replace);
                stack.push_back({x, y});
                count += 1;
            }
        }
    };

    action({x, y}, 0, 0);

    while (not stack.empty()) {
        Coord c = stack.back();
        stack.pop_back();
        action(c, -1, 0);
        action(c, 0, 1);
        action(c, 0, -1);
        action(c, 1, 0);
    }

    return count;
}


void Game::regenerate_map(Platform& pfrm)
{
    TileMap temporary;

    tiles_.for_each([&](Tile& t, int, int) {
        t = Tile(random_choice<int(Tile::sand)>(pfrm));
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

    tiles_.for_each([&](const Tile& tile, s8 x, s8 y) {
        if (tile not_eq Tile::none and tile not_eq Tile::ledge and
            tile not_eq Tile::grass_ledge) {
            temporary.set_tile(x, y, Tile(1));
        } else {
            temporary.set_tile(x, y, Tile(0));
        }
    });

    while (true) {
        const auto x = random_choice(pfrm, TileMap::width);
        const auto y = random_choice(pfrm, TileMap::height);
        if (temporary.get_tile(x, y) not_eq Tile::none) {
            flood_fill(temporary, Tile(2), x, y);
            temporary.for_each([&](const Tile& tile, s8 x, s8 y) {
                if (tile not_eq Tile(2)) {
                    tiles_.set_tile(x, y, Tile::none);
                }
            });
            break;
        } else {
            continue;
        }
    }

    TileMap grass_overlay([&](Tile& t, int, int) {
        if (random_choice<3>(pfrm)) {
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
            bitmask[x][y] += 1 * int(grass_overlay.get_tile(x, y - 1));
            bitmask[x][y] += 2 * int(grass_overlay.get_tile(x + 1, y));
            bitmask[x][y] += 4 * int(grass_overlay.get_tile(x, y + 1));
            bitmask[x][y] += 8 * int(grass_overlay.get_tile(x - 1, y));
        }
    }

    grass_overlay.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::plate) {
            auto match = tiles_.get_tile(x, y);
            u8 val;
            switch (match) {
            case Tile::plate:
                val = int(Tile::grass_plate) + bitmask[x][y];
                tiles_.set_tile(x, y, Tile(val));
                break;

            case Tile::sand:
                val = int(Tile::grass_sand) + bitmask[x][y];
                tiles_.set_tile(x, y, Tile(val));
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
            if (random_choice<8>(pfrm) == 0) {
                tile = Tile::damaged_plate;
            }
        }
    });
}


using MapCoord = Vec2<s8>;
using MapCoordBuf = Buffer<MapCoord, TileMap::tile_count>;


static void
get_free_map_slots(Platform& pfrm, const TileMap& map, MapCoordBuf& output)
{
    map.for_each([&](const Tile& tile, s8 x, s8 y) {
                     if (tile not_eq Tile::none and
                         tile not_eq Tile::ledge and
                         tile not_eq Tile::grass_ledge) {
                         output.push_back({x, y});
                     }
                 });

    for (auto it = output.begin(); it not_eq output.end();) {
        const Tile tile = map.get_tile(it->x, it->y);
        if (not(tile == Tile::sand or (u8(tile) >= u8(Tile::grass_sand) and
                                       u8(tile) < u8(Tile::grass_plate)))) {
            output.erase(it);
        } else {
            ++it;
        }
    }
}


bool Game::respawn_entities(Platform& pfrm)
{
    auto clear_entities = [&](auto& buf) { buf.clear(); };

    enemies_.transform(clear_entities);
    details_.transform(clear_entities);
    effects_.transform(clear_entities);

    Buffer<MapCoord, TileMap::tile_count> free_spots;
    get_free_map_slots(pfrm, tiles_, free_spots);

    if (free_spots.size() < 6) {
        // The randomly generated map is unacceptably tiny! Try again...
        return false;
    }

    auto select_coord = [&]() -> MapCoord* {
        if (not free_spots.empty()) {
            auto result = &free_spots[random_choice(pfrm, free_spots.size())];
            free_spots.erase(result);
            return result;
        } else {
            return nullptr;
        }
    };

    auto pos = [&](const MapCoord* c) {
        return Vec2<float>{static_cast<Float>(c->x * 32),
                           static_cast<Float>(c->y * 24)};
    };

    auto player_coord = select_coord();
    if (player_coord) {
        player_.set_position(pos(player_coord));
    } else {
        return false;
    }

    if (not free_spots.empty()) {
        MapCoord* farthest = free_spots.begin();
        for (auto& elem : free_spots) {
            if (manhattan_length(elem, *player_coord) >
                manhattan_length(*farthest, *player_coord)) {
                farthest = &elem;
            }
        }
        transporter_.set_position(pos(farthest));
        free_spots.erase(farthest);
    }

    if (const auto c = select_coord()) {
        details_.get<0>().push_back(make_entity<ItemChest>(pos(c)));
    }

    return true;
}
