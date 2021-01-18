#pragma once


#include "blind_jump/entity/entity.hpp"
#include "graphics/animation.hpp"


class Game;
class Platform;


template <TextureIndex InitialTexture, u32 AnimLen, u32 AnimInterval>
class TransientEffect : public Entity {
public:
    TransientEffect()
    {
        animation_.bind(sprite_);
    }

    void update(Platform&, Game&, Microseconds dt)
    {
        if (animation_.advance(sprite_, dt) and animation_.done(sprite_)) {
            this->kill();
        }
    }

private:
    Animation<InitialTexture, AnimLen, AnimInterval> animation_;
};
