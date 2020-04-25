#pragma once

#include "entity/enemies/enemy.hpp"


class Player;
class Laser;


class Gatekeeper : public Enemy {
public:
    Gatekeeper(const Vec2<Float>& position);

    void update(Platform&, Game&, Microseconds);

    static constexpr bool multiface_sprite = true;

    auto get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&)
    {
    }


private:
    Sprite head_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
