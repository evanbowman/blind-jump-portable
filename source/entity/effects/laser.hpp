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

    void on_collision(Platform&, Game&, Drone&);
    void on_collision(Platform&, Game&, Turret&);
    void on_collision(Platform&, Game&, Dasher&);
    void on_collision(Platform&, Game&, SnakeHead&);
    void on_collision(Platform&, Game&, SnakeBody&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    Cardinal dir_;
    Microseconds timer_;
    HitBox hitbox_;
};
