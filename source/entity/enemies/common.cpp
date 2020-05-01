#include "common.hpp"
#include "game.hpp"
#include "number/random.hpp"


void on_enemy_destroyed(Platform& pfrm,
                        Game& game,
                        const Vec2<Float>& position,
                        int item_drop_chance,
                        const Item::Type allowed_item_drop[])
{
    game.camera().shake();

    game.effects().spawn<Explosion>(sample<18>(position));

    pfrm.speaker().play_sound("explosion1", 3);


    game.on_timeout(milliseconds(60), [pos = position](Platform&, Game& game) {
        game.effects().spawn<Explosion>(sample<18>(pos));

        // Chaining these functions may uses less of the game's resources
        game.on_timeout(milliseconds(120), [pos = pos](Platform&, Game& game) {
            game.effects().spawn<Explosion>(sample<18>(pos));
        });
    });

    const auto tile_coord = to_tile_coord(position.cast<s32>());
    const auto tile = game.tiles().get_tile(tile_coord.x, tile_coord.y);

    // We do not want to spawn rubble over an empty map tile
    if (is_walkable__fast(tile)) {
        game.on_timeout(milliseconds(200),
                        [pos = position](Platform&, Game& game) {
                            game.details().spawn<Rubble>(pos);
                        });
    }

    if (item_drop_chance) {
        if (random_choice(item_drop_chance) == 0) {

            int count = 0;
            while (true) {
                if (allowed_item_drop[count] == Item::Type::null) {
                    break;
                }
                ++count;
            }

            game.details().spawn<Item>(
                position, pfrm, allowed_item_drop[random_choice(count)]);
        }
    }
}
