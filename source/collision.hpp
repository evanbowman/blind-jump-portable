#pragma once


#include "buffer.hpp"


class ItemChest;
class Critter;
class Player;
class Dasher;
class Probe;


class Collidable {
public:
    virtual ~Collidable()
    {
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
};


template <typename Derived>
class CollidableTemplate : public Collidable {
public:
    void initiate_collision(Collidable& other) override
    {
        other.receive_collision(*static_cast<Derived*>(this));
    }
};


using CollisionSpace = Buffer<Collidable*, 25>;


void check_collisions(CollisionSpace& input);
