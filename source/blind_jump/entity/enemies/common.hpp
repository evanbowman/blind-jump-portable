#pragma once


#include "blind_jump/entity/details/item.hpp"
#include "number/random.hpp"


class Platform;
class Game;


void on_enemy_destroyed(Platform& pfrm,
                        Game& game,
                        int expl_y_offset,
                        const Vec2<Float>& position,
                        int item_drop_chance,
                        const Item::Type allowed_item_drop[],
                        bool create_debris = true);
