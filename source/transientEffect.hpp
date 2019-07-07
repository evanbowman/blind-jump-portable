#pragma once


#include "animation.hpp"
#include "entity.hpp"


class Game;
class Platform;


template <u32 Count, TextureIndex InitialTexture, u32 AnimLen, u32 AnimInterval>
class TransientEffect
    : public Entity<
          TransientEffect<Count, InitialTexture, AnimLen, AnimInterval>,
          Count> {
public:
    void update(Platform&, Game&, Microseconds dt)
    {
        animation_.advance(sprite_, dt);
        if (animation_.done()) {
            this->kill();
        }
    }

private:
    Animation<InitialTexture, AnimLen, AnimInterval> animation_;
};
