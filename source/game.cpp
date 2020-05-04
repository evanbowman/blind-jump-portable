#include "game.hpp"
#include "function.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"
#include "string.hpp"
#include "util.hpp"
#include <algorithm>
#include <iterator>
#include <type_traits>


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


Game::Game(Platform& pfrm) : state_(State::initial())
{
    if (auto sd = pfrm.read_save()) {
        info(pfrm, "loaded existing save file");
        persistent_data_ = *sd;
    } else {
        info(pfrm, "no save file found");
    }

    player_.set_health(persistent_data_.player_health_);

    random_seed() = persistent_data_.seed_;

    pfrm.load_overlay_texture("overlay");

    state_->enter(pfrm, *this);

    if (not inventory().has_item(Item::Type::blaster)) {
        inventory().push_item(pfrm, *this, Item::Type::blaster);
    }
}


HOT void Game::update(Platform& pfrm, Microseconds delta)
{
    for (auto it = deferred_callbacks_.begin();
         it not_eq deferred_callbacks_.end();) {

        it->second -= delta;

        if (not(it->second > 0)) {
            it->first(pfrm, *this);
            it = deferred_callbacks_.erase(it);
        } else {
            ++it;
        }
    }

    auto new_state = state_->update(pfrm, *this, delta);

    if (new_state) {
        state_->exit(pfrm, *this);
        new_state->enter(pfrm, *this);

        state_ = std::move(new_state);
    }
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
    display_buffer.push_back(&player_.weapon().get_sprite());

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


READ_ONLY_DATA
static constexpr const bool boss_level_0[TileMap::width][TileMap::height] = {
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

READ_ONLY_DATA
static constexpr const bool boss_level_1[TileMap::width][TileMap::height] = {
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


struct BossLevel {
    const bool (*map_)[TileMap::height];
    const char* spritesheet_;
};


static const Level boss_0_level = 10;
static const Level boss_1_level = 21;


static const BossLevel* get_boss_level(Level current_level)
{
    switch (current_level) {
    case boss_0_level: {
        static constexpr const BossLevel ret{boss_level_0, "spritesheet_boss0"};
        return &ret;
    }

    case boss_1_level: {
        static constexpr const BossLevel ret{boss_level_1, "spritesheet_boss1"};
        return &ret;
    }

    default:
        return nullptr;
    }
}


bool is_boss_level(Level level)
{
    return get_boss_level(level) not_eq nullptr;
}


static void draw_starfield(Platform& pfrm)
{
    // Redraw the starfield background. Due to memory constraints, the
    // background needs to source its 8x8 pixel tiles from the tile0 texture.
    for (int x = 0; x < 32; ++x) {
        for (int y = 0; y < 32; ++y) {
            pfrm.set_background_tile(x, y, [] {
                if (random_choice<9>()) {
                    return 60;
                } else {
                    if (random_choice<2>()) {
                        return 70;
                    } else {
                        return 71;
                    }
                }
            }());
        }
    }
}


static constexpr const ZoneInfo zone_1{"part I:",
                                       "outer station ruins",
                                       "spritesheet",
                                       "tilesheet",
                                       "tilesheet_top",
                                       "frostellar",
                                       Microseconds{0},
                                       ColorConstant::electric_blue,
                                       ColorConstant::aerospace_orange,
                                       [](Platform& pfrm, Game&) {
                                           draw_starfield(pfrm);

                                           const int x = 16;
                                           const int y = 16;

                                           draw_background_image(
                                               pfrm, 61, x, y, 3, 3);
                                       }};


static constexpr const ZoneInfo zone_2{"part II:",
                                       "the descent",
                                       "spritesheet2",
                                       "tilesheet2",
                                       "tilesheet2_top",
                                       "computations",
                                       seconds(8) + milliseconds(700),
                                       ColorConstant::turquoise_blue,
                                       ColorConstant::safety_orange,
                                       [](Platform& pfrm, Game&) {
                                           draw_starfield(pfrm);

                                           const int x = 16;
                                           const int y = 16;

                                           draw_background_image(
                                               pfrm, 120, x, y, 5, 5);
                                       }};


static constexpr const ZoneInfo zone_3{"part III:",
                                       "moonlight kingdom",
                                       "spritesheet3",
                                       "tilesheet3",
                                       "tilesheet3_top",
                                       "computations",
                                       seconds(8) + milliseconds(700),
                                       ColorConstant::cerulean_blue,
                                       ColorConstant::aerospace_orange,
                                       [](Platform& pfrm, Game&) {
                                           draw_starfield(pfrm);

                                           const int x = 12;
                                           const int y = 12;

                                           draw_background_image(
                                               pfrm, 120, x, y, 9, 9);
                                       }};


const ZoneInfo& zone_info(Level level)
{
    if (level > boss_1_level) {
        return zone_3;
    } else if (level > boss_0_level) {
        return zone_2;
    } else {
        return zone_1;
    }
}


const ZoneInfo& current_zone(Game& game)
{
    return zone_info(game.level());
}


bool operator==(const ZoneInfo& lhs, const ZoneInfo& rhs)
{
    return strcmp(lhs.title_line_2, rhs.title_line_2) == 0;
}


COLD void Game::next_level(Platform& pfrm, std::optional<Level> set_level)
{
    const auto& last_zone = zone_info(level());

    if (set_level) {
        persistent_data_.level_ = *set_level;
    } else {
        persistent_data_.level_ += 1;
    }

    persistent_data_.player_health_ = player_.get_health();
    persistent_data_.seed_ = random_seed();


    pfrm.load_tile0_texture(current_zone(*this).tileset0_name_);
    pfrm.load_tile1_texture(current_zone(*this).tileset1_name_);


    auto boss_level = get_boss_level(level());
    if (boss_level) {
        pfrm.speaker().stop_music();
        pfrm.load_sprite_texture(boss_level->spritesheet_);

    } else {
        pfrm.load_sprite_texture(current_zone(*this).spritesheet_name_);
    }

RETRY:
    Game::regenerate_map(pfrm);

    if (not Game::respawn_entities(pfrm)) {
        warning(pfrm, "Map is too small, regenerating...");
        goto RETRY;
    }

    pfrm.push_tile0_map(tiles_);

    current_zone(*this).generate_background_(pfrm, *this);

    // We're doing this to speed up collision checking with walls. While it
    // might be nice to have more info about the tilemap, it's costly to check
    // all of the enumerations. At this point, we've already pushed the tilemap
    // to the platform for rendering, so we can simplify to just the data that
    // we absolutely need.
    tiles_.for_each([&](Tile& tile, int, int) {
        if (is_walkable__precise(tile)) {
            switch (tile) {
            case Tile::plate:
            case Tile::damaged_plate:
                tile = Tile::plate;
                break;

            default:
                tile = Tile::sand;
                break;
            }
        } else {
            tile = Tile::none;
        }
    });

    const auto player_pos = player_.get_position();
    const auto ssize = pfrm.screen().size();
    camera_.set_position(
        pfrm, {player_pos.x - ssize.x / 2, player_pos.y - float(ssize.y)});

    if ((set_level and *set_level == 0) or
        not(zone_info(level()) == last_zone)) {
        pfrm.write_save(persistent_data_);
    }
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


COLD void Game::seed_map(Platform& pfrm, TileMap& workspace)
{
    if (auto l = get_boss_level(level())) {
        for (int x = 0; x < TileMap::width; ++x) {
            for (int y = 0; y < TileMap::height; ++y) {
                tiles_.set_tile(x, y, static_cast<Tile>(l->map_[x][y]));
            }
        }
    } else {
        tiles_.for_each([&](Tile& t, int, int) {
            t = Tile(random_choice<int(Tile::sand)>());
        });

        for (int i = 0; i < 2; ++i) {
            condense(tiles_, workspace);
        }
    }
}


COLD void Game::regenerate_map(Platform& pfrm)
{
    TileMap temporary;

    seed_map(pfrm, temporary);

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
        if (is_walkable__precise(tile)) {
            temporary.set_tile(x, y, Tile(1));
        } else {
            temporary.set_tile(x, y, Tile(0));
        }
    });

    if (not is_boss_level(level())) {
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
            switch (match) {
            case Tile::plate:
            case Tile::sand:
                grass_overlay.set_tile(
                    x, y, Tile(int(Tile::grass_start) + bitmask[x][y]));
                break;

            case Tile::ledge:
                tiles_.set_tile(x,
                                y,
                                (random_choice<2>()) ? Tile::grass_ledge
                                                     : Tile::grass_ledge_vines);
                grass_overlay.set_tile(x, y, Tile::none);
                break;

            default:
                break;
            }
        }
    });


    if (zone_info(level()) == zone_1) {
        tiles_.for_each([&](Tile& tile, int x, int y) {
            if (tile == Tile::ledge) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate) {
                    tile = Tile::beam_br;
                } else if (tiles_.get_tile(x - 1, y) == Tile::plate) {
                    tile = Tile::beam_bl;
                }
            } else if (tile == Tile::none and
                       tiles_.get_tile(x, y + 1) == Tile::plate) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate) {
                    tile = Tile::beam_ur;
                } else if (tiles_.get_tile(x - 1, y) == Tile::plate) {
                    tile = Tile::beam_ul;
                }
            }
        });
    }

    if (zone_info(level()) == zone_3) {
        tiles_.for_each([&](Tile& tile, int x, int y) {
            if (not is_walkable__precise(tile)) {
                if (is_walkable__precise(tiles_.get_tile(x, y + 1))) {
                    grass_overlay.set_tile(x, y, static_cast<Tile>(17));
                }
            }
        });
    }


    pfrm.push_tile1_map(grass_overlay);


    tiles_.for_each([&](Tile& tile, int x, int y) {
        if (tile == Tile::plate) {
            if (random_choice<7>() == 0) {
                tile = Tile::damaged_plate;
            }
        } else if (tile == Tile::sand) {
            if (random_choice<4>() == 0 and
                grass_overlay.get_tile(x, y) == Tile::none) {
                tile = Tile::sand_sprouted;
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
        if (is_walkable__precise(tile)) {
            output.push_back({x, y});
        }
    });

    for (auto it = output.begin(); it not_eq output.end();) {
        const Tile tile = map.get_tile(it->x, it->y);
        if (not(tile == Tile::sand or tile == Tile::sand_sprouted
                // FIXME-TILES
                //  or
                // (u8(tile) >= u8(Tile::grass_sand) and
                //  u8(tile) < u8(Tile::grass_plate))
                )) {
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
    // We want to spawn enemies based on both the difficulty of level, and the
    // number of free spots that are actually available on the map. Some
    // enemies, like snakes, take up a lot of resources on constrained systems,
    // so we can only display one or _maybe_ two.
    //
    // Some other enemies require a lot of map space to fight effectively, so
    // they are banned from tiny maps.

    const auto density = std::min(0.16f, 0.07f + game.level() * 0.004f);

    constexpr auto max_enemies = 6;

    const int spawn_count =
        std::max(std::min(max_enemies, int(free_spots.size() * density)), 1);

    struct EnemyInfo {
        int min_level_;
        Function<64, void()> spawn_;
        int max_level_ = std::numeric_limits<Level>::max();
        int max_allowed_ = 1000;
    } info[] = {
        {0,
         [&]() {
             spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             if (game.level() > 6) {
                 spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             }
         },
         10},
        {5,
         [&]() { spawn_entity<Dasher>(pfrm, free_spots, game.enemies()); },
         boss_1_level},
        {7,
         [&]() {
             spawn_entity<SnakeHead>(pfrm, free_spots, game.enemies(), game);
         },
         boss_0_level,
         1},
        {0, [&]() { spawn_entity<Turret>(pfrm, free_spots, game.enemies()); }},
        {11,
         [&]() { spawn_entity<Scarecrow>(pfrm, free_spots, game.enemies()); },
         boss_1_level}};


    Buffer<EnemyInfo*, 100> distribution;

    while (not distribution.full()) {
        auto selected = &info[random_choice<sizeof info / sizeof(EnemyInfo)>()];

        if (selected->min_level_ > std::max(Level(0), game.level()) or
            selected->max_level_ < game.level()) {
            continue;
        }

        distribution.push_back(selected);
    }

    int i = 0;
    while (i < spawn_count) {

        auto choice = distribution[random_choice<distribution.capacity()>()];

        if (choice->max_allowed_ == 0) {
            continue;
        }

        ++i;
        choice->max_allowed_--;
        choice->spawn_();
    }
}


static void
spawn_item_chest(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    // Certain items only appear once, and some items are more likely to appear
    // than others. Some items, like hearts and coins, only appear in the
    // outside of item chests, so should not be included.

    auto item = Item::Type::heart;

    // FIXME: this is just debugging code (I added this code just to make items
    // show up, but items won't necessarily be well proportioned in terms of
    // their value, it's just random right now).
    while ((game.inventory().has_item(item) and item_is_persistent(item)) or
           item == Item::Type::heart or item == Item::Type::coin or
           (item == Item::Type::lethargy and
            game.inventory().has_item(Item::Type::lethargy))) {

        item = static_cast<Item::Type>(
            random_choice(static_cast<int>(Item::Type::count)));
    }

    spawn_entity<ItemChest>(pfrm, free_spots, game.details(), item);
}


COLD bool Game::respawn_entities(Platform& pfrm)
{
    auto clear_entities = [&](auto& buf) { buf.clear(); };

    enemies_.transform(clear_entities);
    details_.transform(clear_entities);
    effects_.transform(clear_entities);

    auto free_spots = get_free_map_slots(tiles_);

    const size_t initial_free_spaces = free_spots.size();

    if (free_spots.size() < 6) {
        // The randomly generated map is unacceptably tiny! Try again...
        return false;
    }

    auto player_coord = select_coord(free_spots);
    if (player_coord) {

        player_.move(world_coord(*player_coord));

        // We want to remove adjacent free spaces, so that the player doesn't
        // spawn on top of enemies, and get injured upon entry into a level. The
        // game is meant to be difficult, but not cruel!
        for (auto it = free_spots.begin(); it not_eq free_spots.end();) {
            if (abs(it->x - player_coord->x) < 2 and
                abs(it->y - player_coord->y) < 2) {

                it = free_spots.erase(it);
            } else {
                ++it;
            }
        }

    } else {
        return false;
    }

    auto is_plate = [&](Tile t) { return t == Tile::plate; };
    auto is_sand = [&](Tile t) {
        return t == Tile::sand or t == Tile::sand_sprouted;
    };


    // When the current level is one of the pre-generated boss levels, we do not
    // want to spawn all of the other stuff in the level, just the player.
    if (get_boss_level(level())) {

        MapCoord* farthest = free_spots.begin();
        for (auto& elem : free_spots) {
            if (manhattan_length(elem, *player_coord) >
                manhattan_length(*farthest, *player_coord)) {
                farthest = &elem;
            }
        }
        const auto target = world_coord(*farthest);

        if (level() == boss_0_level) {
            enemies_.spawn<TheFirstExplorer>(target);

        } else {
            enemies_.spawn<Gatekeeper>(target, *this);
        }

        // Place two hearts in the level. The game is supposed to be difficult,
        // but not cruel!
        int heart_count = 2;

        while (true) {
            const s8 x = random_choice<TileMap::width>();
            const s8 y = random_choice<TileMap::height>();

            if (is_plate(tiles_.get_tile(x, y))) {

                auto wc = to_world_coord({x, y});
                wc.x += 16;

                details_.spawn<Item>(wc, pfrm, Item::Type::heart);

                if (--heart_count == 0) {
                    break;
                }
            }
        }

        transporter_.set_position({0, 0});

        return true;
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
        transporter_.set_position(sample<3>({target.x, target.y + 16}));
        free_spots.erase(farthest);
    } else {
        return false;
    }

    // Sometimes for small maps, and always for large maps, place an item chest
    if (random_choice<2>() or initial_free_spaces > 25) {
        spawn_item_chest(pfrm, *this, free_spots);
    }

    spawn_enemies(pfrm, *this, free_spots);

    // Potentially hide some items in far crannies of the map. If
    // there's no sand nearby, and no items eiher, potentially place
    // an item.
    tiles_.for_each([&](Tile t, s8 x, s8 y) {
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
            if (random_choice<3>()) {
                MapCoord c{x, y};

                // NOTE: We want hearts to become less available at higher
                // levels, so that the game naturally becomes more
                // challenging. But for the first few levels, do not make hearts
                // more scarce.
                const int heart_chance =
                    3 + std::max(level() - 4, Level(0)) * 0.2f;

                if (not details_.spawn<Item>(world_coord(c), pfrm, [&] {
                        if (random_choice(heart_chance)) {
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

    // For map locations with nothing nearby, potentially place an item or
    // something
    tiles_.for_each([&](Tile t, s8 x, s8 y) {
        if (is_sand(t)) {
            const auto pos = to_world_coord({x, y});

            bool entity_nearby = false;
            auto check_nearby = [&](auto& entity) {
                if (entity_nearby) {
                    return;
                }
                if (distance(entity.get_position(), pos) < 132) {
                    entity_nearby = true;
                }
            };

            auto check_entities = [&](auto& buf) {
                for (auto& entity : buf) {
                    check_nearby(*entity);
                }
            };

            enemies_.transform(check_entities);
            check_entities(details_.get<Item>());
            check_entities(details_.get<ItemChest>());
            check_nearby(transporter_);

            if (not entity_nearby) {
                int adj_sand_tiles = 0;
                for (int i = x - 1; i < x + 1; ++i) {
                    for (int j = y - 1; j < y + 1; ++j) {
                        if (is_sand(tiles_.get_tile(i, j))) {
                            adj_sand_tiles++;
                        }
                    }
                }

                auto wc = to_world_coord({x, y});
                wc.x += 16; // center the item over the tile

                if (adj_sand_tiles < 1) {
                    // If there are no other adjacent non-edge
                    // tiles, definitely place an item
                    details_.spawn<Item>(wc, pfrm, Item::Type::heart);

                } else if (adj_sand_tiles < 3) {
                    // If there is a low number of adjacent
                    // non-edge tiles, possibly place an item
                    if (random_choice<3>() == 0) {
                        details_.spawn<Item>(wc, pfrm, Item::Type::heart);
                    }
                } else {
                    if (random_choice<5>() == 0) {
                        details_.spawn<Item>(wc, pfrm, Item::Type::heart);
                    }
                }
            }
        }
    });

    int heart_count = 0;
    for (auto& item : details_.get<Item>()) {
        if (item->get_type() == Item::Type::heart) {
            ++heart_count;
        }
    }
    // We don't want to make the levels too easy either
    if (heart_count > 2) {
        const auto item_count = length(details_.get<Item>());

        while (heart_count > 2) {
            auto choice = random_choice(item_count);

            if (auto item = list_ref(details_.get<Item>(), choice)) {
                if ((*item)->get_type() == Item::Type::heart) {
                    (*item)->set_type(Item::Type::coin);
                    --heart_count;
                }
            }
        }
    }

    return true;
}


bool within_view_frustum(const Platform::Screen& screen, const Vec2<Float>& pos)
{
    // FIXME: I thought I had the math correct, but apparantly the view center
    // points to the top left corner of the screen. Ah well...
    const auto view_center = screen.get_view().get_center().cast<s32>();
    const auto view_half_extent = screen.size().cast<s32>() / s32(2);
    Vec2<s32> view_br = {view_center.x + view_half_extent.x * 2,
                         view_center.y + view_half_extent.y * 2};
    return pos.x > view_center.x - 32 and pos.x < view_br.x + 32 and
           pos.y > view_center.y - 32 and pos.y < view_br.y + 32;
}


Game::DeferredCallback screen_flash_animation(int remaining)
{
    return [remain = remaining](Platform& pf, Game& game) {
        if (remain > 0) {
            pf.screen().fade(255 - remain, ColorConstant::silver_white);

            game.on_timeout(milliseconds(20),
                            screen_flash_animation(remain - 1));

        } else {
            pf.screen().fade(0);
        }
    };
}
