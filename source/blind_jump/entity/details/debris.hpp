#pragma once

#include "blind_jump/entity/entity.hpp"


class Debris : public Entity {
public:
    Debris(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds delta);

private:
    Microseconds timer_;
    Vec2<Float> step_vector_;
    u8 rotation_dir_;
};
