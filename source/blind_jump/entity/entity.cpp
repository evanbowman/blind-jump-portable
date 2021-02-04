#include "entity.hpp"


static Entity::Id id_counter_ = 1;


void Entity::override_id(Id id)
{
    id_ = id;

    if (id_counter_ < id) {
        id_counter_ = id + 1;
    }
}


void Entity::reset_ids()
{
    // I'm not actually worried about running out of ids. The real concern, is
    // that one of the players during a multiplayer game ends up with more
    // enemies spawned during a level for one reason or another, and stuff
    // getting glitched up.
    id_counter_ = 1;
}


Entity::Entity() : health_(1), id_(id_counter_++)
{
}


Entity::Entity(Health health) : health_(health), id_(id_counter_++)
{
}


void Entity::update(Platform&, Game&, Microseconds)
{
}


Entity::Health Entity::get_health() const
{
    return health_;
}


bool Entity::alive() const
{
    return health_ not_eq 0;
}


const Sprite& Entity::get_sprite() const
{
    return sprite_;
}


Sprite& Entity::get_sprite()
{
    return sprite_;
}


void Entity::set_position(const Vec2<Float>& position)
{
    position_ = position;
}


void Entity::add_health(Health amount)
{
    health_ += amount;
}


const Vec2<Float>& Entity::get_position() const
{
    return position_;
}
