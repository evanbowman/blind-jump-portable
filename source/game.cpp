#include <algorithm>
#include "game.hpp"


Game::Game(Platform& platform) : player_(Player::pool().get())
{
    details_.get<0>().push_back(ItemChest::pool().get());

    regenerate_map(platform);

    platform.push_map(tiles_);
}


void Game::update(Platform& pfrm, Microseconds delta)
{
    player_->update(pfrm, *this, delta);

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            (*it)->update(pfrm, *this, delta);
        }
    };

    enemies_.transform(update_policy);
    details_.transform(update_policy);
    effects_.transform(update_policy);

    camera_.update(pfrm, delta, player_->get_position());

    display_buffer.push_back(&player_->get_sprite());

    auto render_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            display_buffer.push_back(&(*it)->get_sprite());
        }
    };

    enemies_.transform(render_policy);
    details_.transform(render_policy);
    effects_.transform(render_policy);

    std::sort(display_buffer.begin(), display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });
}


void Game::render(Platform& pfrm)
{
    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }

    display_buffer.clear();
}


static void condense(TileMap& map, TileMap& maptemp) {
    map.for_each([&](const Tile& tile, int x, int y) {
                     uint8_t count = 0;
                     if (map.get_tile(x - 1, y - 1) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x + 1, y - 1) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x - 1, y + 1) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x + 1, y + 1) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x - 1, y) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x + 1, y) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x, y - 1) == Tile::none) {
                         count += 1;
                     }
                     if (map.get_tile(x, y + 1) == Tile::none) {
                         count += 1;
                     }
                     if (tile == Tile::none) {
                         if (count > 3) {
                             maptemp.set_tile(x, y, Tile::plate);
                         } else {
                             maptemp.set_tile(x, y, Tile::none);
                         }
                     } else {
                         if (count > 4) {
                             maptemp.set_tile(x, y, Tile::none);
                         } else {
                             maptemp.set_tile(x, y, Tile::plate);
                         }
                     }
                 });
    maptemp.for_each([&](const Tile& tile, int x, int y) {
                         map.set_tile(x, y, tile);
                     });
}


// Having a whole other tilemap in memory is somewhat costly. But
// there isn't really any other way...
TileMap temporary;


void Game::regenerate_map(Platform& platform)
{
    tiles_.for_each([&](Tile& tile, int, int) {
                    tile = Tile(random_choice<int(Tile::sand)>(platform));
                });

    for (int i = 0; i < 3; ++i) {
        condense(tiles_, temporary);
    }

    tiles_.for_each([&](Tile& tile, int x, int y) {
                        if (tile == Tile::plate and
                            tiles_.get_tile(x - 1, y) not_eq Tile::none and
                            tiles_.get_tile(x + 1, y) not_eq Tile::none and
                            tiles_.get_tile(x, y - 1) not_eq Tile::none and
                            tiles_.get_tile(x, y + 1) not_eq Tile::none) {
                            tile = Tile::sand;
                        } else if (tile == Tile::plate) {
                            if (random_choice<8>(platform) == 0) {
                                tile = Tile::damaged_plate;
                            }
                        }
                    });

    tiles_.for_each([&](Tile& tile, int x, int y) {
                        auto above = tiles_.get_tile(x, y - 1);
                        if (tile == Tile::none and
                            (above == Tile::plate or above == Tile::damaged_plate)) {
                            tile = Tile::ledge;
                        }
                    });
}
