#pragma once


#include "buffer.hpp"


class Critter;
class Turret;
class Player;
class Dasher;
class Probe;
class Item;


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

    virtual void initiate_collision(Collidable&) = 0;

    virtual void receive_collision(Critter&)
    {
    }

    virtual void receive_collision(Player&)
    {
    }

    virtual void receive_collision(Dasher&)
    {
    }

    virtual void receive_collision(Probe&)
    {
    }

    virtual void receive_collision(Turret&)
    {
    }

    virtual void receive_collision(Item&)
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

    void initiate_collision(Collidable& other) override
    {
        other.receive_collision(*static_cast<Derived*>(this));
    }
};


using CollisionSpace = Buffer<Collidable*, 25>;


void check_collisions(CollisionSpace& input);
