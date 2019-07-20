#pragma once


#include "buffer.hpp"


class Platform;
class Critter;
class Turret;
class Player;
class Dasher;
class Probe;
class Item;
class Game;


struct HitBox {
    Vec2<Float>* position_;
    Vec2<s16> size_;
    Vec2<s16> origin_;

    bool overlapping(const HitBox& other) const
    {
        const auto c = center();
        const auto oc = other.center();
        if (c.x < (oc.x + other.size_.x) && (c.x + size_.x) > oc.x &&
            c.y < (oc.y + other.size_.y) && (c.y + size_.y) > oc.y) {
            return true;
        } else {
            return false;
        }
    }

    Vec2<s16> center() const
    {
        Vec2<s16> c;
        c.x = s16(position_->x) - origin_.x;
        c.y = s16(position_->y) - origin_.y;
        return c;
    }
};


class Collidable {
public:
    Collidable(const HitBox& hit_box) : hit_box_(hit_box)
    {
    }

    virtual ~Collidable()
    {
    }

    const HitBox& hit_box() const
    {
        return hit_box_;
    }

    virtual void send_collision(Platform&, Game&, Collidable&) = 0;

    virtual void on_collision(Platform&, Game&, Critter&)
    {
    }

    virtual void on_collision(Platform&, Game&, Player&)
    {
    }

    virtual void on_collision(Platform&, Game&, Dasher&)
    {
    }

    virtual void on_collision(Platform&, Game&, Probe&)
    {
    }

    virtual void on_collision(Platform&, Game&, Turret&)
    {
    }

    virtual void on_collision(Platform&, Game&, Item&)
    {
    }

protected:
    HitBox hit_box_;
};


template <typename Derived> class CollidableTemplate : public Collidable {
public:
    CollidableTemplate(const HitBox& hit_box) : Collidable(hit_box)
    {
    }

    void send_collision(Platform& pf, Game& game, Collidable& other) override
    {
        other.on_collision(pf, game, *static_cast<Derived*>(this));
    }
};


using CollisionSpace = Buffer<Collidable*, 25>;


void check_collisions(Platform& pf, Game& game, CollisionSpace& input);
