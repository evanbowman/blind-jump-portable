#include "entity/entity.hpp"
#include "collision.hpp"


class Turret;
class Dasher;
class SnakeHead;
class SnakeBody;


class Laser : public Entity {
public:

    enum class Direction { up, down, left, right };

    Laser(const Vec2<Float>& position, Direction dir);

    void update(Platform& pf, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, Turret&);
    void on_collision(Platform&, Game&, Dasher&);
    void on_collision(Platform&, Game&, SnakeHead&);
    void on_collision(Platform&, Game&, SnakeBody&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    Direction dir_;
    Microseconds timer_;
    HitBox hitbox_;
};
