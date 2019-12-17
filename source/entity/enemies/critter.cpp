#include "critter.hpp"


Critter::Critter(Platform& pf)
{
    sprite_.set_texture_index(0); // FIXME
}


void Critter::update(Platform& pf, Game&, Microseconds)
{
}


// void Critter::on_collision(Critter&)
// {
//     // TODO
// }
