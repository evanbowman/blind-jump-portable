#include "common.hpp"
#include "blind_jump/game.hpp"
#include "number/random.hpp"


void on_enemy_destroyed(Platform& pfrm,
                        Game& game,
                        int expl_y_offset,
                        const Vec2<Float>& position,
                        int item_drop_chance,
                        const Item::Type allowed_item_drop[],
                        bool create_debris)
{
    game.camera().shake();

    const auto expl_pos = Vec2<Float>{position.x, position.y + expl_y_offset};

    auto dt = pfrm.make_dynamic_texture();
    if (rng::choice<2>(rng::utility_state) and dt) {
        game.effects().spawn<DynamicEffect>(
            Vec2<Float>{expl_pos.x, expl_pos.y - 2},
            *dt,
            milliseconds(90),
            89,
            8);
    } else {
        game.effects().spawn<Explosion>(
            rng::sample<18>(expl_pos, rng::utility_state));

        game.on_timeout(
            pfrm, milliseconds(60), [pos = expl_pos](Platform& pf, Game& game) {
                game.effects().spawn<Explosion>(
                    rng::sample<18>(pos, rng::utility_state));

                // Chaining these functions may uses less of the game's resources
                game.on_timeout(
                    pf, milliseconds(120), [pos = pos](Platform&, Game& game) {
                        game.effects().spawn<Explosion>(
                            rng::sample<18>(pos, rng::utility_state));
                    });
            });
    }


    pfrm.speaker().play_sound("explosion1", 3, position);


    const auto tile_coord = to_tile_coord(position.cast<s32>());
    const auto tile = game.tiles().get_tile(tile_coord.x, tile_coord.y);

    // We do not want to spawn rubble over an empty map tile
    if (create_debris and is_walkable(tile)) {
        game.on_timeout(pfrm,
                        milliseconds(200),
                        [pos = position](Platform& pfrm, Game& game) {
                            game.details().spawn<Rubble>(pos);

                            if (not pfrm.network_peer().is_connected()) {
                                if (length(game.details().get<Debris>()) < 8) {
                                    game.details().spawn<Debris>(pos);
                                    game.details().spawn<Debris>(pos);
                                    game.details().spawn<Debris>(pos);
                                    game.details().spawn<Debris>(pos);
                                } else {
                                    game.details().spawn<Debris>(pos);
                                    game.details().spawn<Debris>(pos);
                                }
                            }
                        });
    } else {
        game.on_timeout(pfrm,
                        milliseconds(200),
                        [pos = position](Platform& pfrm, Game& game) {
                            if (length(game.details().get<Debris>()) < 8) {
                                game.details().spawn<Debris>(pos);
                                game.details().spawn<Debris>(pos);
                                game.details().spawn<Debris>(pos);
                                game.details().spawn<Debris>(pos);
                                game.details().spawn<Debris>(pos);
                            } else {
                                game.details().spawn<Debris>(pos);
                                game.details().spawn<Debris>(pos);
                            }
                        });
    }

    if (item_drop_chance) {
        if (rng::choice(item_drop_chance, rng::critical_state) == 0) {

            int count = 0;
            while (true) {
                if (allowed_item_drop[count] == Item::Type::null) {
                    break;
                }
                ++count;
            }

            auto make_item = [&] {
                return game.details().spawn<Item>(
                    position,
                    pfrm,
                    allowed_item_drop[rng::choice(count, rng::critical_state)]);
            };

            if (count > 0) {
                if (not make_item() and length(game.details().get<Debris>())) {
                    game.details().get<Debris>().pop();
                    make_item();
                }
            }
        }
    }
}
