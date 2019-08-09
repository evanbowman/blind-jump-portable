#include "entity.hpp"


EntityBase::EntityBase() : health_(1)
{
}


EntityBase::EntityBase(Health health) : health_(health)
{
}


void EntityBase::update(Platform&, Game&, Microseconds)
{
}


EntityBase::Health EntityBase::get_health() const
{
    return health_;
}


bool EntityBase::alive() const
{
    return health_ not_eq 0;
}


const Sprite& EntityBase::get_sprite() const
{
    return sprite_;
}


void EntityBase::set_position(const Vec2<Float>& position)
{
    position_ = position;
}


void EntityBase::add_health(Health amount)
{
    health_ += amount;
}


const Vec2<Float>& EntityBase::get_position() const
{
    return position_;
}
