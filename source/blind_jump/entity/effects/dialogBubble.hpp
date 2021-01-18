#pragma once

#include "blind_jump/entity/entity.hpp"


class DialogBubble : public Entity {
public:
    DialogBubble(const Vec2<Float>& position, Entity& owner);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void try_destroy(Entity&);

private:
    enum class State {
        animate_in,
        wait,
        animate_out,
    } state_;

    Microseconds timer_;
    Entity* owner_;
};
