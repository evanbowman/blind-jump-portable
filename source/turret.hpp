#pragma once


#include "animation.hpp"
#include "collision.hpp"
#include "entity.hpp"


class Game;
class Platform;


class Turret : public Entity<Turret, 6>, public CollidableTemplate<Turret> {
public:
    Turret(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:
    Sprite shadow_;
    Animation<TextureMap::turret, 5, Microseconds(50000)> animation_;
    enum class State { closed, opening, open, closing } state_;
};
