#include "entity.hpp"


Entity::Entity() : health_(1)
{
}


Entity::Entity(Health health) : health_(health)
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
