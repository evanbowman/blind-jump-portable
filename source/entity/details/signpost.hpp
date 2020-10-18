#pragma once

#include "entity/entity.hpp"
#include "localeString.hpp"
#include "memory/buffer.hpp"


class Signpost : public Entity {
public:
    enum class Type { memorial };

    Signpost(const Vec2<Float>& position, Type type);

    static constexpr bool has_shadow = true;
    static constexpr bool multiface_shadow = true;
    static constexpr bool multiface_sprite = true;

    Type get_type() const
    {
        return type_;
    }

    void update(Platform&, Game&, Microseconds dt);

    auto get_sprites() const
    {
        Buffer<const Sprite*, 2> result;

        switch (type_) {
        case Type::memorial:
            result.push_back(&sprite_);
            result.push_back(&extra_);
            break;
        }

        return result;
    }

    auto get_shadow() const
    {
        Buffer<const Sprite*, 1> result;

        switch (type_) {
        case Type::memorial:
            break;
        }

        return result;
    }

    const LocaleString* get_dialog() const;

private:
    Sprite extra_;
    Type type_;
};
