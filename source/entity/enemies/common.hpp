#pragma once


#include "entity/details/item.hpp"
#include "number/random.hpp"


class Platform;
class Game;


void on_enemy_destroyed(Platform& pfrm,
                        Game& game,
                        const Vec2<Float>& position,
                        int item_drop_chance,
                        const Item::Type allowed_item_drop[],
                        bool create_debris = true);
