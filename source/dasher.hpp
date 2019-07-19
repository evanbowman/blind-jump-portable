#pragma once

#include "collision.hpp"
#include "entity.hpp"
#include "sprite.hpp"


class Dasher : public Entity<Dasher, 20>, public CollidableTemplate<Dasher> {
public:
    Dasher(const Vec2<Float>& position);

    void update(Platform&, Game&, Microseconds);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

    static constexpr bool multiface_sprite = true;

    std::array<const Sprite*, 2> get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

private:
    Sprite shadow_;
    Sprite head_;
};
