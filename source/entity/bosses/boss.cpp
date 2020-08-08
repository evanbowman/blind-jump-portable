#include "boss.hpp"
#include "conf.hpp"
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
        pf, milliseconds(300), [pos = position](Platform& pf, Game& game) {
            big_explosion(pf, game, pos);
            const auto off = -50.f;

            big_explosion(pf, game, {pos.x - off, pos.y + off});
            big_explosion(pf, game, {pos.x + off, pos.y - off});

            game.on_timeout(
                pf, milliseconds(100), [pos](Platform& pf, Game& game) {
                    for (int i = 0; i <
                                    [&] {
                                        switch (game.difficulty()) {
                                        case Difficulty::count:
                                        case Difficulty::normal:
                                            break;

                                        case Difficulty::survival:
                                        case Difficulty::hard:
                                            return 2;
                                        }
                                        return 3;
                                    }();
                         ++i) {
                        game.details().spawn<Item>(
                            rng::sample<32>(pos, rng::utility_state),
                            pf,
                            Item::Type::heart);
                    }

                    game.transporter().set_position(pos);

                    screen_flash_animation(255)(pf, game);
                });
        });

    pf.speaker().stop_music();
    pf.sleep(10);

    const auto score = Conf(pf).expect<Conf::Integer>("scoring", "boss");

    switch (game.difficulty()) {
    case Difficulty::count:
    case Difficulty::normal:
        game.score() += score;
        break;

    case Difficulty::hard:
    case Difficulty::survival:
        game.score() += score * 1.15f;
        break;
    }
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

        if (not is_walkable__fast(
                game.tiles().get_tile(tile_coord.x, tile_coord.y))) {
            return true;
        }
    }

    return false;
}
