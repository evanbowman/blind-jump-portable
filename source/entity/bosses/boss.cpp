#include "boss.hpp"
#include "game.hpp"
#include "number/random.hpp"


void boss_explosion(Platform& pf, Game& game, const Vec2<Float>& position)
{
    hide_boss_health(game);

    static const auto score = 1000;

    switch (game.difficulty()) {
    case Settings::Difficulty::count:
    case Settings::Difficulty::normal:
        game.score() += score;
        break;

    case Settings::Difficulty::hard:
    case Settings::Difficulty::survival:
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

        if (not is_walkable(
                game.tiles().get_tile(tile_coord.x, tile_coord.y))) {
            return true;
        }
    }

    return false;
}
