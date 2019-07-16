#include "collision.hpp"


void check_collisions(CollisionSpace& input)
{
    for (auto c : input) {
        for (auto o : input) {
            if (o not_eq c) {
                // TODO: if overlapping...
                // c->initiate_collision(*o);
            }
        }
    }
}
