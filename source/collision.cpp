#include "collision.hpp"


void check_collisions(Platform& pf, Game& game, CollisionSpace& input)
{
    const auto count = input.size();
    for (u32 i = 0; i < count; ++i) {
        for (u32 j = i + 1; j < count; ++j) {
            if (input[i]->hit_box().overlapping(input[j]->hit_box())) {
                input[i]->send_collision(pf, game, *input[j]);
                input[j]->send_collision(pf, game, *input[i]);
            }
        }
    }
}
