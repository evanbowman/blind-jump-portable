#pragma once

#include "entity/entity.hpp"


class Terminal : public Entity {
public:
    Terminal(const Vec2<Float>& position);

    static constexpr bool has_shadow = true;

    void update(Platform&, Game&, Microseconds dt);

    const Sprite& get_shadow() const
    {
        return shadow_;
    }

private:
    Sprite shadow_;
};
