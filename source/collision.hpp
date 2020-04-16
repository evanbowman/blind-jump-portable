#pragma once


#include "list.hpp"


struct HitBox {
    Vec2<Float>* position_;

    struct Dimension {
        Vec2<s16> size_;
        Vec2<s16> origin_;
    } dimension_;

    bool overlapping(const HitBox& other) const
    {
        const auto c = center();
        const auto oc = other.center();
        if (c.x < (oc.x + other.dimension_.size_.x) and
            (c.x + dimension_.size_.x) > oc.x and
            c.y < (oc.y + other.dimension_.size_.y) and
            (c.y + dimension_.size_.y) > oc.y) {
            return true;
        } else {
            return false;
        }
    }

    Vec2<s16> center() const
    {
        Vec2<s16> c;
        c.x = s16(position_->x) - dimension_.origin_.x;
        c.y = s16(position_->y) - dimension_.origin_.y;
        return c;
    }
};


class Game;
class Platform;


template <typename A, typename B, typename Pl1, typename Pl2>
void check_collisions(Platform& pf,
                      Game& game,
                      List<A, Pl1>& lhs,
                      List<B, Pl2>& rhs)
{
    for (auto& a : lhs) {
        if (a->visible()) {
            for (auto& b : rhs) {
                if (a->hitbox().overlapping(b->hitbox())) {
                    a->on_collision(pf, game, *b);
                    b->on_collision(pf, game, *a);
                }
            }
        }
    }
}


template <typename A, typename B, typename Pl1>
void check_collisions(Platform& pf, Game& game, A& lhs, List<B, Pl1>& rhs)
{
    for (auto& b : rhs) {
        if (b->visible()) {
            if (lhs.hitbox().overlapping(b->hitbox())) {
                lhs.on_collision(pf, game, *b);
                b->on_collision(pf, game, lhs);
            }
        }
    }
}
