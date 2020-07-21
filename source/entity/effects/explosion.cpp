#include "explosion.hpp"
#include "game.hpp"
#include "number/random.hpp"


Explosion::Explosion(const Vec2<Float>& position)
{
    set_position(position);

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 8});
    sprite_.set_position(position);
}


void big_explosion(Platform& pfrm, Game& game, const Vec2<Float>& position)
{
    for (int i = 0; i < 4; ++i) {
        game.effects().spawn<Explosion>(rng::sample<18>(position));
    }

    game.on_timeout(milliseconds(90), [pos = position](Platform&, Game& game) {
        for (int i = 0; i < 3; ++i) {
            game.effects().spawn<Explosion>(rng::sample<32>(pos));
        }
        game.on_timeout(milliseconds(90), [pos](Platform&, Game& game) {
            for (int i = 0; i < 2; ++i) {
                game.effects().spawn<Explosion>(rng::sample<48>(pos));
            }
            game.on_timeout(milliseconds(90), [pos](Platform&, Game& game) {
                for (int i = 0; i < 1; ++i) {
                    game.effects().spawn<Explosion>(rng::sample<48>(pos));
                }
            });
        });
    });

    game.camera().shake(14);
}


void medium_explosion(Platform& pfrm, Game& game, const Vec2<Float>& position)
{
    game.effects().spawn<Explosion>(rng::sample<18>(position));


    game.on_timeout(milliseconds(60), [pos = position](Platform&, Game& game) {
        game.effects().spawn<Explosion>(rng::sample<18>(pos));

        // Chaining these functions may uses less of the game's resources
        game.on_timeout(milliseconds(120), [pos = pos](Platform&, Game& game) {
            game.effects().spawn<Explosion>(rng::sample<18>(pos));
        });
    });
}
