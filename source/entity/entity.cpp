#include "entity.hpp"


Entity::Entity(const char* name) : health_(1), name_(name)
{
}


Entity::Entity(Health health, const char* name) : health_(health), name_(name)
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
