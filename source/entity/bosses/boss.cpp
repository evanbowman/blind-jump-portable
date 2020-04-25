#include "boss.hpp"
#include "game.hpp"
#include "number/random.hpp"


void boss_explosion(Platform& pf, Game& game, const Vec2<Float>& position)
{
    hide_boss_health(game);

    big_explosion(pf, game, position);

    const auto off = 50.f;

    big_explosion(pf, game, {position.x - off, position.y - off});
    big_explosion(pf, game, {position.x + off, position.y + off});

    game.on_timeout(
        milliseconds(300), [pos = position](Platform& pf, Game& game) {
            big_explosion(pf, game, pos);
            const auto off = -50.f;

            big_explosion(pf, game, {pos.x - off, pos.y + off});
            big_explosion(pf, game, {pos.x + off, pos.y - off});

            game.on_timeout(milliseconds(100), [pos](Platform& pf, Game& game) {
                for (int i = 0; i < 3; ++i) {
                    game.details().spawn<Item>(
                        sample<32>(pos), pf, Item::Type::heart);
                }

                game.transporter().set_position(pos);

                screen_flash_animation(255)(pf, game);
            });
        });

    pf.speaker().stop_music();
    pf.sleep(10);

    game.score() += 1000;

}


bool wall_in_path(const Vec2<Float>& direction,
                  const Vec2<Float> position,
                  Game& game,
                  const Vec2<Float>& destination)
{
    int dist = distance(position, destination);

    for (int i = 24; i < dist; i += 16) {
        Vec2<Float> pos = {position.x + direction.x * i,
                           position.y + direction.y * i};

        auto tile_coord = to_tile_coord(pos.cast<s32>());

        if (not is_walkable(
                game.tiles().get_tile(tile_coord.x, tile_coord.y))) {
            return true;
        }
    }

    return false;
}
