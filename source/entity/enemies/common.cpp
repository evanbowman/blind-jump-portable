#include "common.hpp"
#include "game.hpp"


void on_enemy_destroyed(Game& game, const Vec2<Float>& position)
{
    game.camera().shake();

    game.effects().spawn<Explosion>(sample<18>(position));

    auto deferred_explosion = [pos = position](Platform&, Game& game) {
        game.effects().spawn<Explosion>(sample<18>(pos));
    };

    game.on_timeout(milliseconds(60), deferred_explosion);
    game.on_timeout(milliseconds(180), deferred_explosion);
}
