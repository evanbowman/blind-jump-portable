#pragma once

#include "entity/enemies/enemy.hpp"


class Player;
class Laser;


class Gatekeeper : public Enemy {
public:
    Gatekeeper(const Vec2<Float>& position, Game& game);

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

    void on_death(Platform&, Game&);

private:
    enum class State { idle, jump, airborne, landing };

    Sprite head_;
    State state_;
    Microseconds timer_;
    Vec2<Float> move_vec_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};


class GatekeeperShield : public Enemy {
public:
    GatekeeperShield(const Vec2<Float>& position, int offset);

    void update(Platform&, Game&, Microseconds);

    void on_collision(Platform&, Game&, Laser&);

    void on_death(Platform&, Game&);

private:
    enum class State { orbit, detached };

    Microseconds timer_;
    State state_;
    int offset_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
