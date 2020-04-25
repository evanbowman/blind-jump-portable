#pragma once

#include "number/numeric.hpp"


class Platform;
class Game;


void boss_explosion(Platform& pf, Game& game, const Vec2<Float>& position);


bool wall_in_path(const Vec2<Float>& direction,
                  const Vec2<Float> position,
                  Game& game,
                  const Vec2<Float>& destination);
