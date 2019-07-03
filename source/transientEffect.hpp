#pragma once


#include "entity.hpp"
#include "animation.hpp"


class Game;


template <u32 Count, u32 InitialTexture, u32 AnimLen, u32 AnimInterval>
class TransientEffect :
    public Entity<TransientEffect<Count,
                                  InitialTexture,
                                  AnimLen,
                                  AnimInterval>, Count> {
public:

    TransientEffect()
    {
        sprite_.initialize();
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        animation_.advance(sprite_, dt);
        if (animation_.done()) {
            this->kill();
        }
    }

private:
    Sprite sprite_;
    Animation<InitialTexture, AnimLen, AnimInterval> animation_;
};
