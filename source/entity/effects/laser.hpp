#include "collision.hpp"
#include "entity/entity.hpp"


class Turret;
class Dasher;
class SnakeHead;
class SnakeBody;
class Drone;


class Laser : public Entity {
public:
    Laser(const Vec2<Float>& position, Cardinal dir);

    void update(Platform& pf, Game& game, Microseconds dt);

    template <typename T> void on_collision(Platform&, Game&, T&)
    {
        this->kill();
    }

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    Cardinal dir_;
    Microseconds timer_;
    HitBox hitbox_;
};
