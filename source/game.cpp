#include "game.hpp"
#include "number/random.hpp"
#include "util.hpp"
#include <algorithm>
#include <iterator>
#include <type_traits>


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


Game::Game(Platform& pfrm) : level_(-1), counter_(0), state_(State::initial())
{
    if (auto sd = pfrm.read_save()) {
        info(pfrm, "loaded existing save file");
        save_data_ = *sd;
    } else {
        info(pfrm, "no save file found");
    }

    random_seed() = save_data_.seed_;

    pfrm.load_sprite_texture("bgr_spritesheet");
    pfrm.load_tile_texture("bgr_tilesheet");

    Game::next_level(pfrm);
}


HOT void Game::update(Platform& pfrm, Microseconds delta)
{
    // Every update, advance the random number engine, so that the
    // amount of time spent on a level contributes some entropy to the
    // number stream. This makes the game somewhat less predictable,
    // because, knowing the state of the random number engine, you
    // would have to beat the current level in some microsecond
    // granularity to get to a new level that's possible to
    // anticipate.
    random_value();

    state_ = state_->update(pfrm, delta, *this);
}


HOT void Game::render(Platform& pfrm)
{
    Buffer<const Sprite*, Platform::Screen::sprite_limit> display_buffer;

    auto show_sprite = [&](Entity& e) {
        if (within_view_frustum(pfrm.screen(), e.get_sprite().get_position())) {
            display_buffer.push_back(&e.get_sprite());
            e.mark_visible(true);
        } else {
            e.mark_visible(false);
        }
    };

    auto show_sprites = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            show_sprite(**it);
        }
    };

    // Draw the effects first. Effects are not subject to z-ordering like
    // overworld objects, therefore, faster to draw.
    effects_.transform(show_sprites);
    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }
    display_buffer.clear();

    display_buffer.push_back(&player_.get_sprite());

    Buffer<const Sprite*, 30> shadows_buffer;

    enemies_.transform([&](auto& entity_buf) {
        for (auto& entity : entity_buf) {
            if (within_view_frustum(pfrm.screen(), entity->get_position())) {

                entity->mark_visible(true);

                if constexpr (std::remove_reference<decltype(
                                  *entity)>::type::multiface_sprite) {

                    const auto sprs = entity->get_sprites();

                    for (const auto& spr : sprs) {
                        display_buffer.push_back(spr);
                    }

                    shadows_buffer.push_back(&entity->get_shadow());

                } else {
                    const auto& spr = entity->get_sprite();
                    display_buffer.push_back(&spr);
                    shadows_buffer.push_back(&entity->get_shadow());
                }
            } else {
                entity->mark_visible(false);
            }
        }
    });
    details_.transform(show_sprites);

    std::sort(display_buffer.begin(),
              display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });

    show_sprite(transporter_);

    display_buffer.push_back(&player_.get_shadow());

    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }

    for (auto spr : shadows_buffer) {
        pfrm.screen().draw(*spr);
    }

    display_buffer.clear();
}


static void condense(TileMap& map, TileMap& maptemp)
{
    // At the start, whether each tile is filled or unfilled is
    // completely random. The condense function causes each tile to
    // appear/disappear based on how many neighbors that tile has,
    // which ultimately causes tiles to coalesce into blobs.
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


COLD void Game::next_level(Platform& pfrm)
{
    level_ += 1;

RETRY:
    Game::regenerate_map(pfrm);

    if (not Game::respawn_entities(pfrm)) {
        warning(pfrm, "Map is too small, regenerating...");
        goto RETRY;
    }

    pfrm.push_map(tiles_);

    const auto player_pos = player_.get_position();
    const auto ssize = pfrm.screen().size();
    camera_.set_position(
        pfrm, {player_pos.x - ssize.x / 2, player_pos.y - float(ssize.y)});
}


COLD static u32 flood_fill(TileMap& map, Tile replace, TIdx x, TIdx y)
{
    using Coord = Vec2<s8>;

    Buffer<Coord, TileMap::width * TileMap::height> stack;

    const Tile target = map.get_tile(x, y);

    u32 count = 0;

    const auto action = [&](const Coord& c, TIdx x_off, TIdx y_off) {
        const TIdx x = c.x + x_off;
        const TIdx y = c.y + y_off;
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


COLD void Game::regenerate_map(Platform& pfrm)
{
    TileMap temporary;

    tiles_.for_each(
        [&](Tile& t, int, int) { t = Tile(random_choice<int(Tile::sand)>()); });

    for (int i = 0; i < 3; ++i) {
        condense(tiles_, temporary);
    }

    // Remove tiles from edges of the map. Some platforms,
    // particularly consoles, have automatic tile-wrapping, and we
    // don't want to deal with having to support wrapping in all the
    // game logic.
    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (x == 0 or x == TileMap::width - 1 or y == 0 or
            y == TileMap::height - 1) {
            tile = Tile::none;
        }
    });

    // Create a mask of the tileset by filling the temporary tileset
    // with all walkable tiles from the tilemap.
    tiles_.for_each([&](const Tile& tile, TIdx x, TIdx y) {
        if (tile not_eq Tile::none and tile not_eq Tile::ledge and
            tile not_eq Tile::grass_ledge) {
            temporary.set_tile(x, y, Tile(1));
        } else {
            temporary.set_tile(x, y, Tile(0));
        }
    });

    // Pick a random filled tile, and flood-fill around that tile in
    // the map, to produce a single connected component.
    while (true) {
        const auto x = random_choice(TileMap::width);
        const auto y = random_choice(TileMap::height);
        if (temporary.get_tile(x, y) not_eq Tile::none) {
            flood_fill(temporary, Tile(2), x, y);
            temporary.for_each([&](const Tile& tile, TIdx x, TIdx y) {
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
        if (random_choice<3>()) {
            t = Tile::none;
        } else {
            t = Tile::plate;
        }
    });

    for (int i = 0; i < 2; ++i) {
        condense(grass_overlay, temporary);
    }

    // All tiles with four neighbors become sand tiles.
    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::plate and
            tiles_.get_tile(x - 1, y) not_eq Tile::none and
            tiles_.get_tile(x + 1, y) not_eq Tile::none and
            tiles_.get_tile(x, y - 1) not_eq Tile::none and
            tiles_.get_tile(x, y + 1) not_eq Tile::none) {
            tile = Tile::sand;
        }
    });

    // Add ledge tiles to empty locations, where the y-1 tile is non-empty.
    tiles_.for_each([&](Tile& tile, int x, int y) {
        auto above = tiles_.get_tile(x, y - 1);
        if (tile == Tile::none and
            (above == Tile::plate or above == Tile::damaged_plate)) {
            tile = Tile::ledge;
        }
    });

    // Crop the grass overlay tileset to fit the target tilemap.
    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::none) {
            grass_overlay.set_tile(x, y, Tile::none);
        }
    });

    u8 bitmask[TileMap::width][TileMap::height];
    // After running the algorithm below, the bitmask will contain
    // correct tile indices, such that all the grass tiles are
    // smoothly connected.
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
            if (random_choice<8>() == 0) {
                tile = Tile::damaged_plate;
            }
        }
    });
}


using MapCoord = Vec2<TIdx>;
using MapCoordBuf = Buffer<MapCoord, TileMap::tile_count>;


COLD static MapCoordBuf get_free_map_slots(const TileMap& map)
{
    MapCoordBuf output;

    map.for_each([&](const Tile& tile, TIdx x, TIdx y) {
        if (tile not_eq Tile::none and tile not_eq Tile::ledge and
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

    return output;
}


COLD static std::optional<MapCoord> select_coord(MapCoordBuf& free_spots)
{
    if (not free_spots.empty()) {
        auto choice = random_choice(free_spots.size());
        auto it = &free_spots[choice];
        const auto result = *it;
        free_spots.erase(it);
        return result;
    } else {
        return {};
    }
}


template <typename T> struct Type {
    using value = T;
};


static Vec2<Float> world_coord(const MapCoord& c)
{
    return Vec2<Float>{static_cast<Float>(c.x * 32) + 16,
                       static_cast<Float>(c.y * 24)};
}


template <typename Type, typename Group, typename... Params>
void spawn_entity(Platform& pf,
                  MapCoordBuf& free_spots,
                  Group& group,
                  Params&&... params)
{
    if (const auto c = select_coord(free_spots)) {
        if (not group.template spawn<Type>(world_coord(*c), params...)) {
            warning(pf, "spawn failed: entity buffer full");
        }
    } else {
        warning(pf, "spawn failed: out of coords");
    }
}


COLD static void
spawn_enemies(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    spawn_entity<Dasher>(pfrm, free_spots, game.enemies());
    spawn_entity<SnakeHead>(pfrm, free_spots, game.enemies(), game);

    for (int i = 0; i < 2; ++i) {
        spawn_entity<Turret>(pfrm, free_spots, game.enemies());
    }
}


COLD bool Game::respawn_entities(Platform& pfrm)
{
    auto clear_entities = [&](auto& buf) { buf.clear(); };

    enemies_.transform(clear_entities);
    details_.transform(clear_entities);
    effects_.transform(clear_entities);

    auto free_spots = get_free_map_slots(tiles_);

    if (free_spots.size() < 6) {
        // The randomly generated map is unacceptably tiny! Try again...
        return false;
    }

    auto player_coord = select_coord(free_spots);
    if (player_coord) {
        player_.move(world_coord(*player_coord));
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
        const auto target = world_coord(*farthest);
        transporter_.set_position({target.x, target.y + 16});
        free_spots.erase(farthest);
    } else {
        return false;
    }

    spawn_entity<ItemChest>(pfrm, free_spots, details_);

    spawn_enemies(pfrm, *this, free_spots);

    // Potentially hide some items in far crannies of the map. If
    // there's no sand nearby, and no items eiher, potentially place
    // an item.
    tiles_.for_each([&](Tile t, s8 x, s8 y) {
        auto is_plate = [&](Tile t) {
            return t == Tile::plate or
                   (t >= Tile::grass_plate and t < Tile::grass_ledge);
        };
        auto is_sand = [&](Tile t) {
            return t == Tile::sand or
                   (t >= Tile::grass_sand and t < Tile::grass_plate);
        };
        if (is_plate(t)) {
            for (int i = x - 1; i < x + 2; ++i) {
                for (int j = y - 1; j < y + 2; ++j) {
                    const auto curr = tiles_.get_tile(i, j);
                    if (is_sand(curr)) {
                        return;
                    }
                }
            }
            for (auto& item : details_.get<Item>()) {
                if (manhattan_length(item->get_position(),
                                     to_world_coord({x, y})) < 64) {
                    return;
                }
            }
            if (random_choice<2>()) {
                MapCoord c{x, y};
                if (not details_.spawn<Item>(world_coord(c), pfrm, [] {
                        if (random_choice<4>()) {
                            return Item::Type::coin;
                        } else {
                            return Item::Type::heart;
                        }
                    }())) {
                    // TODO: log error?
                }
            }
        }
    });

    return true;
}


bool within_view_frustum(const Platform::Screen& screen, const Vec2<Float>& pos)
{
    const auto view_center = screen.get_view().get_center().cast<s32>();
    const auto view_half_extent = screen.size().cast<s32>() / s32(2);
    Vec2<s32> view_br = {view_center.x + view_half_extent.x * 2,
                         view_center.y + view_half_extent.y * 2};
    return pos.x > view_center.x - 32 and pos.x < view_br.x and
           pos.y > view_center.y - 32 and pos.y < view_br.y;
}
